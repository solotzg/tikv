use kvproto::{metapb, raft_cmdpb, raft_serverpb};
use std::collections::VecDeque;

type TiFlashServerPtr = *const u8;
type RegionId = u64;
pub type SnapKVData = VecDeque<(Vec<u8>, Vec<u8>)>;

pub enum TiFlashApplyRes {
    None,
    Persist,
    NotFound,
}

impl From<u32> for TiFlashApplyRes {
    fn from(o: u32) -> Self {
        match o {
            0 => TiFlashApplyRes::None,
            1 => TiFlashApplyRes::Persist,
            2 => TiFlashApplyRes::NotFound,
            _ => unreachable!(),
        }
    }
}

#[repr(C)]
pub struct TiFlashRaftProxy {
    pub check_sum: u64,
}

pub struct SnapshotData {
    _data: (Vec<BaseBuffView>, Vec<BaseBuffView>),
    pub view: SnapshotDataView,
}

pub fn gen_snap_kv_data(snap: &SnapKVData) -> SnapshotData {
    let mut keys = Vec::<BaseBuffView>::with_capacity(snap.len());
    let mut vals = Vec::<BaseBuffView>::with_capacity(snap.len());

    for (k, v) in snap {
        keys.push(BaseBuffView {
            data: k.as_ptr(),
            len: k.len() as u64,
        });
        vals.push(BaseBuffView {
            data: v.as_ptr(),
            len: v.len() as u64,
        });
    }
    let keys_ptr = keys.as_ptr();
    let keys_len = keys.len();
    let vals_ptr = vals.as_ptr();
    SnapshotData {
        _data: (keys, vals),
        view: SnapshotDataView {
            keys: keys_ptr,
            vals: vals_ptr,
            len: keys_len as u64,
        },
    }
}

#[repr(C)]
pub struct SnapshotDataView {
    keys: *const BaseBuffView,
    vals: *const BaseBuffView,
    len: u64,
}

#[repr(C)]
pub struct BaseBuff {
    inner: *const (),
    data: *const u8,
    len: u64,
}

impl Drop for BaseBuff {
    fn drop(&mut self) {
        (get_tiflash_server_helper().gc_buff)(self as *mut _ as *const _);
    }
}

#[repr(C)]
pub struct BaseBuffView {
    data: *const u8,
    len: u64,
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct RaftCmdHeader {
    region_id: u64,
    index: u64,
    term: u64,
}

impl RaftCmdHeader {
    pub fn new(region_id: u64, index: u64, term: u64) -> Self {
        RaftCmdHeader {
            region_id,
            index,
            term,
        }
    }
}

struct ProtoMsgBaseBuff {
    _data: Vec<u8>,
    buff_view: BaseBuffView,
}

impl ProtoMsgBaseBuff {
    fn new<T: protobuf::Message>(msg: &T) -> Self {
        let v = msg.write_to_bytes().unwrap();
        let ptr = v.as_ptr();
        let len = v.len() as u64;
        ProtoMsgBaseBuff {
            _data: v,
            buff_view: BaseBuffView { data: ptr, len },
        }
    }
}

#[repr(C)]
pub struct TiFlashServerHelper {
    inner: TiFlashServerPtr,
    gc_buff: extern "C" fn(*const BaseBuff),
    handle_write_raft_cmd: extern "C" fn(TiFlashServerPtr, BaseBuffView, RaftCmdHeader) -> u32,
    handle_admin_raft_cmd:
        extern "C" fn(TiFlashServerPtr, BaseBuffView, BaseBuffView, RaftCmdHeader) -> u32,
    handle_apply_snapshot: extern "C" fn(
        TiFlashServerPtr,
        BaseBuffView,
        u64,
        SnapshotDataView,
        SnapshotDataView,
        SnapshotDataView,
        u64,
        u64,
    ),
    atomic_update_proxy: extern "C" fn(TiFlashServerPtr, *const TiFlashRaftProxy),
    handle_destroy: extern "C" fn(TiFlashServerPtr, RegionId),
    //
    magic_number: u32,
    version: u32,
}

unsafe impl Send for TiFlashServerHelper {}

pub static mut TIFLASH_SERVER_HELPER_PTR: u64 = 0;

pub fn get_tiflash_server_helper() -> &'static TiFlashServerHelper {
    return unsafe { &(*(TIFLASH_SERVER_HELPER_PTR as *const TiFlashServerHelper)) };
}

pub fn get_tiflash_server_helper_mut() -> &'static mut TiFlashServerHelper {
    return unsafe { &mut (*(TIFLASH_SERVER_HELPER_PTR as *mut TiFlashServerHelper)) };
}

impl TiFlashServerHelper {
    pub fn handle_write_raft_cmd(
        &self,
        requests: &raft_cmdpb::RaftCmdRequest,
        header: RaftCmdHeader,
    ) -> TiFlashApplyRes {
        let res = (self.handle_write_raft_cmd)(
            self.inner,
            ProtoMsgBaseBuff::new(requests).buff_view,
            header,
        );
        TiFlashApplyRes::from(res)
    }

    pub unsafe fn atomic_update_proxy(&mut self, proxy: *const TiFlashRaftProxy) {
        (self.atomic_update_proxy)(self.inner, proxy);
    }

    pub fn check(&self) {
        assert_eq!(std::mem::align_of::<Self>(), std::mem::align_of::<u64>());
        assert_eq!(self.magic_number, 0x13579BDF);
        assert_eq!(self.version, 1);
    }

    pub fn handle_admin_raft_cmd(
        &self,
        req: &raft_cmdpb::AdminRequest,
        resp: &raft_cmdpb::AdminResponse,
        header: RaftCmdHeader,
    ) -> TiFlashApplyRes {
        let res = (self.handle_admin_raft_cmd)(
            self.inner,
            ProtoMsgBaseBuff::new(req).buff_view,
            ProtoMsgBaseBuff::new(resp).buff_view,
            header,
        );
        TiFlashApplyRes::from(res)
    }

    pub fn handle_apply_snapshot(
        &self,
        region: &metapb::Region,
        peer_id: u64,
        lock_cf_snap: SnapshotDataView,
        write_cf_snap: SnapshotDataView,
        default_cf_snap: SnapshotDataView,
        index: u64,
        term: u64,
    ) {
        (self.handle_apply_snapshot)(
            self.inner,
            ProtoMsgBaseBuff::new(region).buff_view,
            peer_id,
            lock_cf_snap,
            write_cf_snap,
            default_cf_snap,
            index,
            term,
        );
    }

    pub fn handle_destroy(&self, region_id: RegionId) {
        (self.handle_destroy)(self.inner, region_id);
    }
}
