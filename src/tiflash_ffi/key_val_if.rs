use crate::tiflash_ffi::invoke::{get_tiflash_server_helper, BaseBuffView};
use engine::{Peekable, WriteOptions, CF_DEFAULT, CF_LOCK, CF_RAFT, CF_TIFLASH, CF_WRITE, DB};
use std::sync::Arc;

enum DBGetValueCfState {
    OK,
    Error,
    KeyNotFound,
}

impl Into<i32> for DBGetValueCfState {
    fn into(self) -> i32 {
        match self {
            DBGetValueCfState::OK => 0,
            DBGetValueCfState::Error => 1,
            DBGetValueCfState::KeyNotFound => 2,
        }
    }
}

#[repr(C)]
pub struct DBGetValueCfRes {
    inner: *const u8,
    state: i32,
}

#[no_mangle]
pub extern "C" fn ffi_handle_put_tiflash_cf(
    proxy_ptr: TiFlashRaftProxyPtr,
    key_: BaseBuffView,
    value_: BaseBuffView,
    sync: u8,
) -> i32 {
    unsafe {
        let key = key_.to_slice();
        let value = value_.to_slice();

        let handle = (*proxy_ptr).storage.cf_handle(CF_TIFLASH).unwrap();

        let option = {
            let mut option = WriteOptions::new();
            option.set_sync(sync != 0);
            option
        };

        match (*proxy_ptr)
            .storage
            .put_cf_opt(handle, key.as_ref(), value.as_ref(), &option)
        {
            Ok(_) => DBGetValueCfState::OK.into(),
            _ => DBGetValueCfState::Error.into(),
        }
    }
}

#[no_mangle]
pub extern "C" fn ffi_handle_get_value_cf(
    proxy_ptr: TiFlashRaftProxyPtr,
    key_: BaseBuffView,
    cf_: u8,
) -> DBGetValueCfRes {
    unsafe {
        let cf = match cf_ {
            0 => CF_TIFLASH,
            1 => CF_LOCK,
            2 => CF_WRITE,
            3 => CF_DEFAULT,
            4 => CF_RAFT,
            _ => unreachable!(),
        };
        let key = key_.to_slice();

        return match (*proxy_ptr).storage.get_value_cf(cf, key) {
            Ok(Some(v)) => DBGetValueCfRes {
                inner: get_tiflash_server_helper().gen_cpp_string(BaseBuffView {
                    data: v.as_ptr(),
                    len: v.len() as u64,
                }),
                state: DBGetValueCfState::OK.into(),
            },
            Ok(None) => DBGetValueCfRes {
                inner: std::ptr::null(),
                state: DBGetValueCfState::KeyNotFound.into(),
            },
            Err(_) => DBGetValueCfRes {
                inner: std::ptr::null(),
                state: DBGetValueCfState::Error.into(),
            },
        };
    }
}

pub struct TiFlashRaftProxy {
    pub storage: Arc<DB>,
}

type TiFlashRaftProxyPtr = *const TiFlashRaftProxy;

#[repr(C)]
pub struct TiFlashRaftProxyHelper {
    proxy_ptr: TiFlashRaftProxyPtr,
    handle_put_tiflash_cf:
        extern "C" fn(TiFlashRaftProxyPtr, BaseBuffView, BaseBuffView, u8) -> i32,
    handle_get_value_cf: extern "C" fn(TiFlashRaftProxyPtr, BaseBuffView, u8) -> DBGetValueCfRes,
}

impl TiFlashRaftProxyHelper {
    pub fn new(proxy: &TiFlashRaftProxy) -> Self {
        TiFlashRaftProxyHelper {
            proxy_ptr: proxy,
            handle_put_tiflash_cf: ffi_handle_put_tiflash_cf,
            handle_get_value_cf: ffi_handle_get_value_cf,
        }
    }
}
