// Copyright 2017 TiKV Project Authors. Licensed under Apache-2.0.

#[cfg(unix)]
mod imp {
    use engine_rocks::RocksEngine;
    use engine_traits::KvEngines;
    use raftstore::tiflash_ffi::get_tiflash_server_helper;

    #[allow(dead_code)]
    pub fn wait_for_signal(_engines: Option<KvEngines<RocksEngine, RocksEngine>>) {
        use std::thread;
        use std::time::Duration;
        loop {
            // hacked by solotzg.
            if get_tiflash_server_helper().handle_check_terminated() {
                break;
            }
            thread::sleep(Duration::from_millis(200));
        }
    }
}

#[cfg(not(unix))]
mod imp {
    use engine_rocks::RocksEngine;
    use engine_traits::KvEngines;

    pub fn wait_for_signal(_: Option<KvEngines<RocksEngine, RocksEngine>>) {}
}

pub use self::imp::wait_for_signal;
