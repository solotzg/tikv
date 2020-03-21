// Copyright 2017 TiKV Project Authors. Licensed under Apache-2.0.

#[cfg(unix)]
mod imp {
    use engine::Engines;
    use tikv::tiflash_ffi::invoke::get_tiflash_server_helper;

    #[allow(dead_code)]
    pub fn handle_signal(_engines: Option<Engines>) {
        use std::thread;
        use std::time::Duration;
        loop {
            // hacked by solotzg.
            if get_tiflash_server_helper().handle_check_terminated() {
                break;
            }
            thread::sleep(Duration::from_millis(500));
        }
    }
}

#[cfg(not(unix))]
mod imp {
    use engine::Engines;

    pub fn handle_signal(_: Option<Engines>) {}
}

pub use self::imp::handle_signal;
