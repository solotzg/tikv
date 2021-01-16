#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <vector>

#include "ColumnFamily.h"
#include "Common.h"

namespace kvrpcpb {
class ReadIndexResponse;
class ReadIndexRequest;
} // namespace kvrpcpb

namespace DB {

class Region;
class RegionManager;
using RegionPtr = std::shared_ptr<Region>;
using DataBaseRaftProxyPtr = void *;
struct DataBaseSnapshot;

class GlobalContext;
struct DataBaseServer;

class Exception : public std::exception {
 public:
  Exception(const std::string & msg, int code = 0) : _msg(msg), _code(code) {}
  virtual const char * what() const throw() override {
    return "Exception";
  }
 private:
  std::string _msg;
  Exception * _pNested;
  int _code;
};

enum class DataBaseApplyRes : uint32_t {
  None = 0,
  Persist,
  NotFound,
};

enum class WriteCmdType : uint8_t {
  Put = 0,
  Del,
};

extern "C" {

using DataBaseRawString = std::string *;

struct BaseBuffView {
  const char * data;
  const uint64_t len;

  BaseBuffView(const std::string & s) : data(s.data()), len(s.size()) {}
  BaseBuffView(const char * data_, const uint64_t len_) : data(data_), len(len_) {}
  BaseBuffView(std::string_view view) : data(view.data()), len(view.size()) {}
  operator std::string_view() const { return std::string_view(data, len); }
};

struct SnapshotView {
  const BaseBuffView * keys;
  const BaseBuffView * vals;
  const ColumnFamilyType cf;
  const uint64_t len = 0;
};

struct SnapshotViewArray {
  const SnapshotView * views;
  const uint64_t len = 0;
};

struct RaftCmdHeader {
  uint64_t region_id;
  uint64_t index;
  uint64_t term;
};

struct WriteCmdsView {
  const BaseBuffView * keys;
  const BaseBuffView * vals;
  const WriteCmdType * cmd_types;
  const ColumnFamilyType * cmd_cf;
  const uint64_t len;
};

struct FsStats {
  uint64_t used_size;
  uint64_t avail_size;
  uint64_t capacity_size;

  uint8_t ok;

  FsStats() { std::memset(this, 0, sizeof(*this)); }
};

enum class FileEncryptionRes : uint8_t {
  Disabled = 0,
  Ok,
  Error,
};

enum class EncryptionMethod : uint8_t {
  Unknown = 0,
  Plaintext = 1,
  Aes128Ctr = 2,
  Aes192Ctr = 3,
  Aes256Ctr = 4,
};

const char * IntoEncryptionMethodName(EncryptionMethod);

struct FileEncryptionInfo;

enum class RaftProxyStatus : uint8_t {
  Idle = 0,
  Running = 1,
  Stop = 2,
};

using BatchReadIndexRes = std::unique_ptr<std::vector<std::pair<kvrpcpb::ReadIndexResponse, uint64_t>>>;
static_assert(std::is_same_v<BatchReadIndexRes::pointer, BatchReadIndexRes::element_type *>);
struct CppStrVecView;

struct DataBaseRaftProxyHelper {
 public:
  RaftProxyStatus getProxyStatus() const;
  bool checkEncryptionEnabled() const;
  EncryptionMethod getEncryptionMethod() const;
  FileEncryptionInfo getFile(std::string_view) const;
  FileEncryptionInfo newFile(std::string_view) const;
  FileEncryptionInfo deleteFile(std::string_view) const;
  FileEncryptionInfo linkFile(std::string_view, std::string_view) const;
  FileEncryptionInfo renameFile(std::string_view, std::string_view) const;
  kvrpcpb::ReadIndexResponse readIndex(const kvrpcpb::ReadIndexRequest &) const;
  BatchReadIndexRes batchReadIndex(const std::vector<kvrpcpb::ReadIndexRequest> &) const;

 private:
  DataBaseRaftProxyPtr proxy_ptr;
  uint8_t (* fn_handle_get_proxy_status)(DataBaseRaftProxyPtr);
  uint8_t (* fn_is_encryption_enabled)(DataBaseRaftProxyPtr);
  EncryptionMethod (* fn_encryption_method)(DataBaseRaftProxyPtr);
  FileEncryptionInfo (* fn_handle_get_file)(DataBaseRaftProxyPtr, BaseBuffView);
  FileEncryptionInfo (* fn_handle_new_file)(DataBaseRaftProxyPtr, BaseBuffView);
  FileEncryptionInfo (* fn_handle_delete_file)(DataBaseRaftProxyPtr, BaseBuffView);
  FileEncryptionInfo (* fn_handle_link_file)(DataBaseRaftProxyPtr, BaseBuffView, BaseBuffView);
  FileEncryptionInfo (* fn_handle_rename_file)(DataBaseRaftProxyPtr, BaseBuffView, BaseBuffView);
  BatchReadIndexRes::pointer (* fn_handle_batch_read_index)(DataBaseRaftProxyPtr, CppStrVecView);
};

enum class DataBaseStatus : uint8_t {
  Idle = 0,
  Running,
  Stopped,
};

enum class RawCppPtrType : uint32_t {
  None = 0,
  String,
  PreHandledTiKVSnapshot,
  DataBaseSnapshot,
  PreHandledDataBaseSnapshot,
  SplitKeys,
  Unknown,
};

struct PreHandledTiKVSnapshot {
  RegionPtr region;
};

struct RawCppPtr {
  void * ptr;
  RawCppPtrType type;

  RawCppPtr(void * ptr_ = nullptr, RawCppPtrType type_ = RawCppPtrType::None) : ptr(ptr_), type(type_) {}
  RawCppPtr(const RawCppPtr &) = delete;
  RawCppPtr(RawCppPtr &&) = delete;
};

struct CppStrWithView {
  RawCppPtr inner;
  BaseBuffView view;

  CppStrWithView() : inner(nullptr, RawCppPtrType::None), view(nullptr, 0) {}
  CppStrWithView(std::string && v) : inner(new std::string(std::move(v)), RawCppPtrType::String),
                                     view(*reinterpret_cast<DataBaseRawString>(inner.ptr)) {}
  CppStrWithView(const CppStrWithView &) = delete;
};

struct CppStrVecView {
  const BaseBuffView * view;
  uint64_t len;
};

struct CppStrVec {
  std::vector<std::string> data;
  std::vector<BaseBuffView> view;
  CppStrVec(std::vector<std::string> && data_) : data(std::move(data_)) { updateView(); }
  CppStrVec(const CppStrVec &) = delete;
  void updateView() {
    view.clear();
    view.reserve(data.size());
    for (const auto & e : data) {
      view.emplace_back(e);
    }
  }
  CppStrVecView intoOuterView() const { return {view.data(), view.size()}; }
};

struct SerializeDataBaseSnapshotRes {
  uint8_t ok;
  uint64_t key_count;
  uint64_t total_size;
};

struct PreHandledDataBaseSnapshot {
  RegionPtr region;
};

struct GetRegionApproximateSizeKeysRes {
  uint8_t ok;
  uint64_t size;
  uint64_t keys;
};

struct SplitKeys {
  std::vector<std::string> data;
  std::vector<BaseBuffView> view;
  SplitKeys(std::vector<std::string> && data_) : data(std::move(data_)) {}
};

struct SplitKeysWithView {
  RawCppPtr inner;
  BaseBuffView * view{nullptr};
  uint64_t len{0};

  SplitKeysWithView(std::vector<std::string> && data) : inner(new SplitKeys(std::move(data)),
                                                              RawCppPtrType::SplitKeys) {
    updateView();
  }

  void updateView() {
    auto keys = reinterpret_cast<SplitKeys *>(inner.ptr);
    for (const auto & e : keys->data) {
      keys->view.emplace_back(e);
    }
    view = keys->view.data();
    len = keys->view.size();
  }
};

struct SplitKeysRes {
  uint8_t ok;
  uint64_t size;
  uint64_t keys;
  SplitKeysWithView split_keys;
};

struct CheckerConfig {
  uint64_t max_size;
  uint64_t split_size;
  uint64_t batch_split_limit;
};

struct DataBaseServerHelper {
  uint32_t magic_number; // use a very special number to check whether this struct is legal
  uint32_t version;      // version of function interface
  //

  DataBaseServer * inner;
  RawCppPtr (* fn_gen_cpp_string)(BaseBuffView);
  DataBaseApplyRes (* fn_handle_write_raft_cmd)(const DataBaseServer *, WriteCmdsView, RaftCmdHeader);
  DataBaseApplyRes (* fn_handle_admin_raft_cmd)(const DataBaseServer *, BaseBuffView, BaseBuffView, RaftCmdHeader);
  void (* fn_atomic_update_proxy)(DataBaseServer *, DataBaseRaftProxyHelper *);
  void (* fn_handle_destroy)(DataBaseServer *, RegionId);
  DataBaseApplyRes (* fn_handle_ingest_sst)(DataBaseServer *, SnapshotViewArray, RaftCmdHeader);
  uint8_t (* fn_handle_check_terminated)(DataBaseServer *);
  FsStats (* fn_handle_compute_fs_stats)(DataBaseServer *);
  DataBaseStatus (* fn_handle_get_database_status)(DataBaseServer *);
  RawCppPtr (* fn_pre_handle_tikv_snapshot)(DataBaseServer *,
                                            BaseBuffView,
                                            uint64_t,
                                            SnapshotViewArray,
                                            uint64_t,
                                            uint64_t);
  void (* fn_apply_pre_handled_snapshot)(DataBaseServer *, void *, RawCppPtrType);
  void * _fn_none;
  void (* gc_raw_cpp_ptr)(DataBaseServer *, void *, RawCppPtrType);

  BatchReadIndexRes::pointer (* fn_gen_batch_read_index_res)(uint64_t);
  void (* fn_insert_batch_read_index_resp)(BatchReadIndexRes::pointer, BaseBuffView, uint64_t);

  uint8_t (* is_database_snapshot)(DataBaseServer *, BaseBuffView path);
  RawCppPtr (* gen_database_snapshot)(DataBaseServer *, RaftCmdHeader);
  SerializeDataBaseSnapshotRes (* serialize_database_snapshot_into)(DataBaseServer *,
                                                                    DataBaseSnapshot *,
                                                                    BaseBuffView path);
  RawCppPtr (* pre_handle_database_snapshot)(DataBaseServer *,
                                             BaseBuffView,
                                             uint64_t,
                                             uint64_t,
                                             uint64_t,
                                             BaseBuffView);

  GetRegionApproximateSizeKeysRes (* get_region_approximate_size_keys)(DataBaseServer *,
                                                                       uint64_t,
                                                                       BaseBuffView,
                                                                       BaseBuffView);
  SplitKeysRes (* scan_split_keys)(DataBaseServer *, uint64_t, BaseBuffView, BaseBuffView, CheckerConfig);
};

void print_raftstore_proxy_version();
void run_raftstore_proxy_ffi(int argc, const char ** argv, const DataBaseServerHelper *);

}

struct DataBaseServer {
  GlobalContext * context{nullptr};
  DataBaseRaftProxyHelper * proxy_helper{nullptr};
  std::atomic<DataBaseStatus> status{DataBaseStatus::Idle};
};

RawCppPtr GenCppRawString(BaseBuffView);
DataBaseApplyRes HandleAdminRaftCmd(const DataBaseServer * server,
                                    BaseBuffView req_buff,
                                    BaseBuffView resp_buff,
                                    RaftCmdHeader header);
DataBaseApplyRes HandleWriteRaftCmd(const DataBaseServer * server, WriteCmdsView req_buff, RaftCmdHeader header);
void AtomicUpdateProxy(DataBaseServer * server, DataBaseRaftProxyHelper * proxy);
void HandleDestroy(DataBaseServer * server, RegionId region_id);
DataBaseApplyRes HandleIngestSST(DataBaseServer * server, SnapshotViewArray snaps, RaftCmdHeader header);
uint8_t HandleCheckTerminated(DataBaseServer * server);
FsStats HandleComputeFsStats(DataBaseServer * server);
DataBaseStatus HandleGetDataBaseStatus(DataBaseServer * server);
RawCppPtr PreHandleTiKVSnapshot(
    DataBaseServer * server,
    BaseBuffView region_buff,
    uint64_t peer_id,
    SnapshotViewArray snaps,
    uint64_t index,
    uint64_t term);
void ApplyPreHandledSnapshot(DataBaseServer * server, void * res, RawCppPtrType type);
void GcRawCppPtr(DataBaseServer *, void *, RawCppPtrType);
BatchReadIndexRes::pointer GenBatchReadIndexRes(uint64_t cap);
void InsertBatchReadIndexResp(BatchReadIndexRes::pointer, BaseBuffView, uint64_t);
RawCppPtr GenDataBaseSnapshot(DataBaseServer *, RaftCmdHeader);
SerializeDataBaseSnapshotRes SerializeDataBaseSnapshotInto(DataBaseServer *, DataBaseSnapshot *, BaseBuffView);
uint8_t IsDataBaseSnapshot(DataBaseServer *, BaseBuffView);
RawCppPtr PreHandleDataBaseSnapshot(DataBaseServer *, BaseBuffView, uint64_t, uint64_t, uint64_t, BaseBuffView);
void ApplyPreHandledTiKVSnapshot(DataBaseServer * server, PreHandledTiKVSnapshot *);
void ApplyPreHandledDataBaseSnapshot(DataBaseServer *, PreHandledDataBaseSnapshot *);

GetRegionApproximateSizeKeysRes GetRegionApproximateSizeKeys(DataBaseServer *, uint64_t, BaseBuffView, BaseBuffView);
SplitKeysRes ScanSplitKeys(DataBaseServer *, uint64_t, BaseBuffView, BaseBuffView, CheckerConfig);

} // namespace DB
