use crate::raftstore::store::keys;
use engine::rocks::{ColumnFamilyOptions, SeekKey, SstFileReader};
use engine::{CF_DEFAULT, CF_LOCK, CF_WRITE};
use kvproto::{metapb, raft_cmdpb};
use std::collections::VecDeque;

type TiFlashServerPtr = *const u8;
type RegionId = u64;
pub type SnapshotKV = VecDeque<(Vec<u8>, Vec<u8>)>;
pub type SnapshotKVView = (Vec<BaseBuffView>, Vec<BaseBuffView>);

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

pub fn gen_snap_kv_data_from_sst(cf_file_path: &str) -> SnapshotKV {
    let mut cf_snap = SnapshotKV::new();
    let mut sst = SstFileReader::new(ColumnFamilyOptions::default());
    sst.open(cf_file_path).unwrap();
    {
        sst.verify_checksum().unwrap();
        let mut it = sst.iter();
        if it.seek(SeekKey::Start).unwrap() {
            loop {
                let ori_key = keys::origin_key(it.key());
                let ori_val = it.value();
                cf_snap.push_back((ori_key.to_vec(), ori_val.to_vec()));
                let ss = it.next().unwrap();
                if !ss {
                    break;
                }
            }
        }
    }
    cf_snap
}

pub enum WriteCmdType {
    Put,
    Del,
}

#[derive(Copy, Clone)]
pub enum WriteCmdCf {
    Lock,
    Write,
    Default,
}

pub fn name_to_cf(cf: &str) -> WriteCmdCf {
    if cf.is_empty() {
        return WriteCmdCf::Default;
    }
    if cf == CF_LOCK {
        return WriteCmdCf::Lock;
    } else if cf == CF_WRITE {
        return WriteCmdCf::Write;
    } else if cf == CF_DEFAULT {
        return WriteCmdCf::Default;
    }
    unreachable!()
}

#[repr(C)]
pub struct WriteCmdsView {
    keys: *const BaseBuffView,
    vals: *const BaseBuffView,
    cmd_types: *const u8,
    cf: *const u8,
    len: u64,
}

impl Into<u8> for WriteCmdType {
    fn into(self) -> u8 {
        return match self {
            WriteCmdType::Put => 0,
            WriteCmdType::Del => 1,
        };
    }
}

impl Into<u8> for WriteCmdCf {
    fn into(self) -> u8 {
        return match self {
            WriteCmdCf::Lock => 0,
            WriteCmdCf::Write => 1,
            WriteCmdCf::Default => 2,
        };
    }
}

#[derive(Default)]
pub struct WriteCmds {
    keys: Vec<BaseBuffView>,
    vals: Vec<BaseBuffView>,
    cmd_type: Vec<u8>,
    cf: Vec<u8>,
}

impl WriteCmds {
    pub fn with_capacity(cap: usize) -> WriteCmds {
        WriteCmds {
            keys: Vec::<BaseBuffView>::with_capacity(cap),
            vals: Vec::<BaseBuffView>::with_capacity(cap),
            cmd_type: Vec::<u8>::with_capacity(cap),
            cf: Vec::<u8>::with_capacity(cap),
        }
    }

    pub fn new() -> WriteCmds {
        WriteCmds::default()
    }

    pub fn push(&mut self, key: &[u8], val: &[u8], cmd_type: WriteCmdType, cf: &str) {
        self.keys.push(BaseBuffView {
            data: key.as_ptr(),
            len: key.len() as u64,
        });
        self.vals.push(BaseBuffView {
            data: val.as_ptr(),
            len: val.len() as u64,
        });
        self.cmd_type.push(cmd_type.into());
        self.cf.push(name_to_cf(cf).into());
    }

    pub fn len(&self) -> usize {
        return self.cmd_type.len();
    }

    fn gen_view(&self) -> WriteCmdsView {
        WriteCmdsView {
            keys: self.keys.as_ptr(),
            vals: self.vals.as_ptr(),
            cmd_types: self.cmd_type.as_ptr(),
            cf: self.cf.as_ptr(),
            len: self.cmd_type.len() as u64,
        }
    }
}

pub fn gen_snap_kv_data_view(snap: &SnapshotKV) -> SnapshotKVView {
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

    (keys, vals)
}

#[repr(C)]
pub struct SnapshotView {
    keys: *const BaseBuffView,
    vals: *const BaseBuffView,
    cf: u8,
    len: u64,
}

#[repr(C)]
pub struct SnapshotViewArray {
    views: *const SnapshotView,
    len: u64,
}

#[derive(Default)]
pub struct SnapshotHelper {
    cf_snaps: Vec<(WriteCmdCf, SnapshotKV)>,
    kv_view: Vec<SnapshotKVView>,
    snap_view: Vec<SnapshotView>,
}

impl SnapshotHelper {
    pub fn add_cf_snap(&mut self, cf_type: WriteCmdCf, snap_kv: SnapshotKV) {
        self.cf_snaps.push((cf_type, snap_kv));
    }

    pub fn gen_snapshot_view(&mut self) -> SnapshotViewArray {
        let len = self.cf_snaps.len();
        self.kv_view.clear();
        self.snap_view.clear();

        for i in 0..len {
            self.kv_view
                .push(gen_snap_kv_data_view(&self.cf_snaps[i].1));
        }

        for i in 0..len {
            self.snap_view.push(SnapshotView {
                keys: self.kv_view[i].0.as_ptr(),
                vals: self.kv_view[i].1.as_ptr(),
                len: self.kv_view[i].0.len() as u64,
                cf: self.cf_snaps[i].0.clone().into(),
            });
        }
        SnapshotViewArray {
            views: self.snap_view.as_ptr(),
            len: self.snap_view.len() as u64,
        }
    }
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
    handle_write_raft_cmd: extern "C" fn(TiFlashServerPtr, WriteCmdsView, RaftCmdHeader) -> u32,
    handle_admin_raft_cmd:
        extern "C" fn(TiFlashServerPtr, BaseBuffView, BaseBuffView, RaftCmdHeader) -> u32,
    handle_apply_snapshot:
        extern "C" fn(TiFlashServerPtr, BaseBuffView, u64, SnapshotViewArray, u64, u64),
    atomic_update_proxy: extern "C" fn(TiFlashServerPtr, *const TiFlashRaftProxy),
    handle_destroy: extern "C" fn(TiFlashServerPtr, RegionId),
    handle_ingest_sst: extern "C" fn(TiFlashServerPtr, SnapshotViewArray, RaftCmdHeader),
    handle_check_terminated: extern "C" fn(TiFlashServerPtr) -> u8,

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
        cmds: &WriteCmds,
        header: RaftCmdHeader,
    ) -> TiFlashApplyRes {
        let res = (self.handle_write_raft_cmd)(self.inner, cmds.gen_view(), header);
        TiFlashApplyRes::from(res)
    }

    pub unsafe fn atomic_update_proxy(&mut self, proxy: *const TiFlashRaftProxy) {
        (self.atomic_update_proxy)(self.inner, proxy);
    }

    pub fn check(&self) {
        assert_eq!(std::mem::align_of::<Self>(), std::mem::align_of::<u64>());
        const MAGIC_NUMBER: u32 = 0x13579BDF;
        const VERSION: u32 = 4;

        if self.magic_number != MAGIC_NUMBER {
            eprintln!(
                "TiFlash Proxy FFI magic number not match: expect {} got {}",
                MAGIC_NUMBER, self.magic_number
            );
            std::process::exit(-1);
        } else if self.version != VERSION {
            eprintln!(
                "TiFlash Proxy FFI version not match: expect {} got {}",
                VERSION, self.version
            );
            std::process::exit(-1);
        }
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
        snaps: SnapshotViewArray,
        index: u64,
        term: u64,
    ) {
        (self.handle_apply_snapshot)(
            self.inner,
            ProtoMsgBaseBuff::new(region).buff_view,
            peer_id,
            snaps,
            index,
            term,
        );
    }

    pub fn handle_ingest_sst(&self, snaps: SnapshotViewArray, header: RaftCmdHeader) {
        (self.handle_ingest_sst)(self.inner, snaps, header);
    }

    pub fn handle_destroy(&self, region_id: RegionId) {
        (self.handle_destroy)(self.inner, region_id);
    }

    pub fn handle_check_terminated(&self) -> bool {
        (self.handle_check_terminated)(self.inner) == 1
    }
}
