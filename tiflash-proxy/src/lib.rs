// Copyright 2019 TiKV Project Authors. Licensed under Apache-2.0.

#[macro_use(
    slog_error,
    slog_warn,
    slog_info,
    slog_crit,
    slog_log,
    slog_kv,
    slog_b,
    slog_record,
    slog_record_static,
    kv
)]
extern crate slog;
#[macro_use]
extern crate slog_global;

#[macro_use]
mod setup;
mod server;
mod signal_handler;
mod tiflash_raft_proxy;
pub use tiflash_raft_proxy::print_tiflash_proxy_version;
pub use tiflash_raft_proxy::run_tiflash_proxy_ffi;

fn proxy_version_info() -> String {
    let fallback = "Unknown (env var does not exist when building)";
    format!(
        "\nRelease Version:   {}\
         \nGit Commit Hash:   {}\
         \nGit Commit Branch: {}\
         \nUTC Build Time:    {}\
         \nRust Version:      {}",
        env!("CARGO_PKG_VERSION"),
        option_env!("PROXY_BUILD_GIT_HASH").unwrap_or(fallback),
        option_env!("PROXY_BUILD_GIT_BRANCH").unwrap_or(fallback),
        option_env!("PROXY_BUILD_TIME").unwrap_or(fallback),
        option_env!("PROXY_BUILD_RUSTC_VERSION").unwrap_or(fallback),
    )
}

fn log_proxy_info() {
    info!("Welcome To TiFlash Raft Proxy");
    for line in proxy_version_info().lines() {
        info!("{}", line);
    }
}
