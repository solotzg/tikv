use crate::store::{Callback, RaftRouter};
use engine_rocks::RocksEngine;
use futures::Future;
use kvproto::kvrpcpb::{ReadIndexRequest, ReadIndexResponse};
use kvproto::raft_cmdpb::{CmdType, RaftCmdRequest, RaftRequestHeader, Request as RaftRequest};
use tikv_util::future::paired_future_callback;

pub trait ReadIndex {
    fn batch_read_index(&self, req: Vec<ReadIndexRequest>) -> Vec<(ReadIndexResponse, u64)>;
}

pub struct ReadIndexClient {
    pub router: RaftRouter<RocksEngine>,
}

impl ReadIndex for ReadIndexClient {
    fn batch_read_index(&self, req_vec: Vec<ReadIndexRequest>) -> Vec<(ReadIndexResponse, u64)> {
        debug!("batch_read_index start"; "size"=>req_vec.len(), "request"=>?req_vec);
        let mut router_cb_vec = Vec::with_capacity(req_vec.len());
        for req in &req_vec {
            let region_id = req.get_context().get_region_id();
            let mut cmd = RaftCmdRequest::default();
            {
                let mut header = RaftRequestHeader::default();
                let mut inner_req = RaftRequest::default();
                inner_req.set_cmd_type(CmdType::ReadIndex);
                header.set_region_id(region_id);
                header.set_peer(req.get_context().get_peer().clone());
                header.set_region_epoch(req.get_context().get_region_epoch().clone());
                cmd.set_header(header);
                cmd.set_requests(vec![inner_req].into());
            }

            let (cb, future) = paired_future_callback();

            if let Err(_) = self
                .router
                .send_raft_command_with_cb(cmd, Callback::Read(cb))
            {
                let mut resp = ReadIndexResponse::default();
                resp.set_region_error(Default::default());
                return vec![(resp, region_id)];
            }
            router_cb_vec.push((future, region_id));
        }

        let mut read_index_res = Vec::with_capacity(req_vec.len());

        for (future, region_id) in router_cb_vec {
            let future = future.map(move |mut v| {
                let mut resp = ReadIndexResponse::default();
                if v.response.get_header().has_error() {
                    resp.set_region_error(v.response.mut_header().take_error());
                } else {
                    let raft_resps = v.response.get_responses();
                    if raft_resps.len() != 1 {
                        error!(
                            "invalid read index response";
                            "region_id" => region_id,
                            "response" => ?raft_resps
                        );
                        resp.mut_region_error().set_message(format!(
                            "Internal Error: invalid response: {:?}",
                            raft_resps
                        ));
                    } else {
                        let read_index = raft_resps[0].get_read_index().get_read_index();
                        resp.set_read_index(read_index);
                    }
                }
                resp
            });

            let resp = future.wait().unwrap_or({
                let mut resp = ReadIndexResponse::default();
                resp.set_region_error(Default::default());
                resp
            });
            read_index_res.push((resp, region_id));
            if read_index_res.last().unwrap().0.has_region_error() {
                return read_index_res;
            }
        }
        debug!("batch_read_index success"; "response"=>?read_index_res);
        read_index_res
    }
}
