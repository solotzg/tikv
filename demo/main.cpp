#include <cstdio>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <thread>
#include <csignal>

#include "ProxyFFI.h"
#include "Context.h"
#include "GrpcService.h"
#include "ScopeGuard.h"

namespace DB {

LogOutput log_output(LogType::Debug);

static const char * ENGINE_RELEASE_VERSION = "v4.1.0";
static const char * ENGINE_COMMIT_HASH = "233333333";

struct DataBaseProxyConfig {
  static const std::string config_prefix;
  std::vector<const char *> args;
  std::unordered_map<std::string, std::string> val_map;

  DataBaseProxyConfig(std::string data_dir,
                      std::string engine_addr,
                      std::string addr,
                      std::string status_addr,
                      std::string pd_endpoints,
                      std::string proxy_config_path) {
    val_map["--engine-version"] = ENGINE_RELEASE_VERSION;
    val_map["--engine-git-hash"] = ENGINE_COMMIT_HASH;
    val_map["--data-dir"] = std::move(data_dir);
    val_map["--engine-addr"] = std::move(engine_addr);
    val_map["--addr"] = std::move(addr);
    val_map["--status-addr"] = std::move(status_addr);
    val_map["--pd-endpoints"] = std::move(pd_endpoints);
    if (!proxy_config_path.empty())
      val_map["--log-file"] = std::move(proxy_config_path);

    args.push_back("Raft Proxy");
    for (const auto & v : val_map) {
      args.push_back(v.first.data());
      args.push_back(v.second.data());
    }
  }
};

volatile std::sig_atomic_t SignalStatus = 0;

static void SignalHandler(int signal) {
  INFO("Got signal " << signal);
  SignalStatus = signal;
}

std::mutex global_mutex;

std::lock_guard<std::mutex> GenGlobalLock() {
  return std::lock_guard(global_mutex);
}

template<typename Fn, Fn fn, typename... Args>
std::result_of_t<Fn(Args...)> wrapper(Args... args) {
  auto _ = GenGlobalLock();
  return fn(std::forward<Args>(args)...);
}
#define WRAPIT(FUNC) wrapper<decltype(&FUNC), &FUNC>

int run(const std::string & base_path,
        const std::string & address,
        const std::string & engine_addr,
        const std::string & status_addr,
        const std::string & pd_addr,
        const std::string & proxy_config_path) {

  print_raftstore_proxy_version();

  signal(SIGINT, SignalHandler);
  signal(SIGTERM, SignalHandler);

  const std::string ProxyDataPath = base_path + fs::path::preferred_separator + "proxy";
  const std::string DataPath = base_path + fs::path::preferred_separator + "col-store";
  const std::string RegionDataPath = DataPath + fs::path::preferred_separator + "region-data";

  DataBaseProxyConfig config(ProxyDataPath, engine_addr, address, status_addr, pd_addr, proxy_config_path);
  GlobalContext global_context;
  DataBaseServer database_instance_wrap{};
  DataBaseServerHelper helper{
      // a special number, also defined in proxy
      .magic_number = 0x13579BDF,
      .version = 500000,
      .inner = &database_instance_wrap,
      .fn_gen_cpp_string = GenCppRawString,
      .fn_handle_write_raft_cmd = WRAPIT(HandleWriteRaftCmd),
      .fn_handle_admin_raft_cmd = WRAPIT(HandleAdminRaftCmd),
      .fn_atomic_update_proxy = AtomicUpdateProxy,
      .fn_handle_destroy = WRAPIT(HandleDestroy),
      .fn_handle_ingest_sst = WRAPIT(HandleIngestSST),
      .fn_handle_check_terminated = HandleCheckTerminated,
      .fn_handle_compute_fs_stats = WRAPIT(HandleComputeFsStats),
      .fn_handle_get_database_status = HandleGetDataBaseStatus,
      .fn_pre_handle_tikv_snapshot = WRAPIT(PreHandleTiKVSnapshot),
      .fn_apply_pre_handled_snapshot = WRAPIT(ApplyPreHandledSnapshot),
      ._fn_none = nullptr,
      .gc_raw_cpp_ptr = GcRawCppPtr,
      .fn_gen_batch_read_index_res = GenBatchReadIndexRes,
      .fn_insert_batch_read_index_resp = InsertBatchReadIndexResp,
      .is_database_snapshot = WRAPIT(IsDataBaseSnapshot),
      .gen_database_snapshot = WRAPIT(GenDataBaseSnapshot),
      .serialize_database_snapshot_into = WRAPIT(SerializeDataBaseSnapshotInto),
      .pre_handle_database_snapshot = WRAPIT(PreHandleDataBaseSnapshot),
      .get_region_approximate_size_keys = WRAPIT(GetRegionApproximateSizeKeys),
      .scan_split_keys = WRAPIT(ScanSplitKeys),
  };

  auto proxy_runner = std::thread([&helper, &config]() {
    INFO("start raft proxy");
    run_raftstore_proxy_ffi(config.args.size(), config.args.data(), &helper);
  });

  while (!database_instance_wrap.proxy_helper)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

  if (database_instance_wrap.proxy_helper->checkEncryptionEnabled()) {
    auto method = database_instance_wrap.proxy_helper->getEncryptionMethod();
    INFO("encryption is enabled, method is " << IntoEncryptionMethodName(method));
    ERROR("this demo has not support encryption");
    exit(-1);
  } else {
    INFO("encryption is disabled");
  }

  SCOPE_EXIT({
               INFO("let proxy shutdown");
               database_instance_wrap.status = DataBaseStatus::Stopped;
               database_instance_wrap.context = nullptr;
               INFO("wait for proxy thread to join");
               proxy_runner.join();
               INFO("proxy thread is joined");
             });

  {
    INFO("init global context");
    global_context.init(DataPath, database_instance_wrap.proxy_helper);
  }

  std::unique_ptr<grpc::Server> grpc_server;
  std::unique_ptr<FlashService> flash_service;
  {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(engine_addr, grpc::InsecureServerCredentials());
    flash_service = std::make_unique<FlashService>(&global_context);
    builder.SetOption(grpc::MakeChannelArgumentOption("grpc.http2.min_ping_interval_without_data_ms", 10 * 1000));
    builder.SetOption(grpc::MakeChannelArgumentOption("grpc.http2.min_time_between_pings_ms", 10 * 1000));
    builder.RegisterService(flash_service.get());
    INFO("Flash service registered");
    builder.SetMaxReceiveMessageSize(-1);
    builder.SetMaxSendMessageSize(-1);
    grpc_server = builder.BuildAndStart();
    INFO("Flash grpc server listening on [" << engine_addr << "]");
  }
  SCOPE_EXIT({
               gpr_timespec deadline{5, 0, GPR_TIMESPAN};
               INFO("begin to shut down flash grpc server");
               grpc_server->Shutdown(deadline);
               grpc_server->Wait();
               grpc_server.reset();
               INFO("Shut down flash grpc server");
               INFO("Begin to shut down flash service");
               flash_service.reset();
               INFO("Shut down flash service");
             });

  {
    database_instance_wrap.context = &global_context;
    database_instance_wrap.status = DataBaseStatus::Running;
  }

  while (database_instance_wrap.proxy_helper->getProxyStatus() == RaftProxyStatus::Idle)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

  while (!DB::SignalStatus)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

  if (DB::SignalStatus == 543210) {
    constexpr int sleep_time = 1000;
    INFO("got special SignalStatus, start to sleep for " << sleep_time << "ms");
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
    INFO("forcely exit -1");
    exit(-1);
  }

  {
    INFO("stop global context");
    global_context.stop();
  }

  INFO("wait proxy to stop all services");
  while (database_instance_wrap.proxy_helper->getProxyStatus() != RaftProxyStatus::Stop)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  INFO("all services in proxy are stopped");
  return 0;
}

int Main(int argc, char ** argv) {
  if (argc == 2) {
    if (std::strcmp(argv[1], "-V") == 0 || std::strcmp(argv[1], "--version") == 0) {
      printf("Engine Commit Hash:%s\n", ENGINE_COMMIT_HASH);
      printf("Release Version:   %s\n", ENGINE_RELEASE_VERSION);
      print_raftstore_proxy_version();
      return 0;
    }
  }

  if (argc <= 5) {
    WARN("Usage: {bin} --version");
    WARN("       {bin} -V");
    WARN("       {bin} base-path address engine-addr status-addr pd-addr");
    WARN("       {bin} base-path address engine-addr status-addr pd-addr proxy-config-path");
    return 0;
  }
  // "/tmp/__column_storage"  172.16.5.81:20420 172.16.5.81:20321 172.16.5.81:20180 172.16.5.81:6508
  // "/tmp/__column_storage2" 172.16.5.81:20520 172.16.5.81:20521 172.16.5.81:20580 172.16.5.81:6508
  std::string proxy_config_path;
  if (argc == 7)
    proxy_config_path = argv[6];
  run(argv[1], argv[2], argv[3], argv[4], argv[5], proxy_config_path);
  return 0;
}
}

int main(int argc, char ** argv) {
  return DB::Main(argc, argv);
}