#include <sys/statvfs.h>
#include <iostream>
#include <memory>
#include <kvproto/raft_cmdpb.pb.h>

#include "ProxyFFI.h"
#include "Codec.h"
#include "Context.h"
#include "CopHelper.h"

namespace DB {

const std::string ColumnFamilyName::Lock = "lock";
const std::string ColumnFamilyName::Default = "default";
const std::string ColumnFamilyName::Write = "write";

namespace ErrorCodes {
extern const int LOGICAL_ERROR = 1;
}

ColumnFamilyType NameToCF(const std::string & cf) {
  if (cf.empty() || cf == ColumnFamilyName::Default)
    return ColumnFamilyType::Default;
  if (cf == ColumnFamilyName::Lock)
    return ColumnFamilyType::Lock;
  if (cf == ColumnFamilyName::Write)
    return ColumnFamilyType::Write;
  throw Exception("Unsupported cf name " + cf, ErrorCodes::LOGICAL_ERROR);
}

const std::string & CFToName(const ColumnFamilyType type) {
  switch (type) {
    case ColumnFamilyType::Default:return ColumnFamilyName::Default;
    case ColumnFamilyType::Write:return ColumnFamilyName::Write;
    case ColumnFamilyType::Lock:return ColumnFamilyName::Lock;
    default:
      throw Exception("Can not tell cf type " + std::to_string(static_cast<uint8_t>(type)),
                      ErrorCodes::LOGICAL_ERROR);
  }
}

RawCppPtr GenCppRawString(BaseBuffView view) {
  return RawCppPtr(view.len ? new std::string(view.data, view.len) : nullptr, RawCppPtrType::String);
}

static_assert(alignof(DataBaseServerHelper) == alignof(void *));

const char * to_string(WriteCmdType tp) {
  static const char * Names[] = {
      "Put",
      "Del",
  };
  return Names[static_cast<uint8_t>(tp)];
}

struct DataBaseSnapshot {
  explicit DataBaseSnapshot(RegionData && data_) : data(std::move(data_)) {}
  SerializeDataBaseSnapshotRes serializeInto(std::string_view path);
  static const std::string SnapFlag;
 private:
  RegionData data;
};

const std::string DataBaseSnapshot::SnapFlag = "this is database snapshot";

const TableId Region::SchemaColTableID = 12345;

void Region::execWriteRaftCmd(const WriteCmdsView & cmds, RaftCmdHeader header) {
  if (header.index <= appliedIndex())
    return;
  for (auto i = 0; i < cmds.len; ++i) {
    if (cmds.cmd_cf[i] != ColumnFamilyType::Write) {
      ERROR("not support write to cf " << CFToName(cmds.cmd_cf[i]));
      continue;
    }
    TRACE("modify table " << table_id);
    switch (cmds.cmd_types[i]) {
      case WriteCmdType::Put: {
        switch (table_id) {
          case SchemaColTableID: {
            auto m = static_cast<EngineModifyType>(cmds.vals[i].data[0]);
            switch (m) {
              case EngineModifyType::Del: {
                TRACE("try delete record");
                data.remove(std::string(cmds.keys[i]));
                break;
              }
              case EngineModifyType::Add: {
                auto col_id = *reinterpret_cast<const int64_t *>(cmds.vals[i].data + 1);
                auto col_val = *reinterpret_cast<const int64_t *>(cmds.vals[i].data + 1 + 8);
                TRACE("try put col_id " << col_id << " col_val " << col_val);
                auto it = data.data.find(std::string(cmds.keys[i]));
                if (it == data.data.end())
                  it = data.insert(cmds.keys[i], "");
                auto val = TableColValueWrap(it->second);
                auto delta = val.Add(col_id, col_val);
                data.size += delta;
                break;
              }
            }
            break;
          }
          default: {
            data.insert(cmds.keys[i], cmds.vals[i]);
            TRACE(__FUNCTION__ << " region " << regionId() << " put key: {size: " << cmds.keys[i].len
                               << "}, value: {size: "
                               << cmds.vals[i].len
                               << ", data: " << std::string_view(cmds.vals[i]) << "} at index " << header.index);
            break;
          }
        }
        break;
      }
      case WriteCmdType::Del: {
        data.remove(std::string(cmds.keys[i]));
        TRACE(__FUNCTION__ << " region " << regionId() << " del key: {size: " << cmds.keys[i].len << "} at index "
                           << header.index);
        break;
      }
    }
  }
  apply_state.set_applied_index(header.index);
}

static std::optional<metapb::Peer> findPeerByStore(const metapb::Region & region, uint64_t store_id) {
  for (const auto & peer : region.peers()) {
    if (peer.store_id() == store_id)
      return peer;
  }
  return std::nullopt;
}

static std::optional<metapb::Peer> findPeer(const metapb::Region & region, uint64_t peer_id) {
  for (const auto & peer : region.peers()) {
    if (peer.id() == peer_id)
      return peer;
  }
  return std::nullopt;
}

std::vector<RegionPtr> Region::splitInto(const raft_cmdpb::AdminResponse & response) {
  std::vector<RegionPtr> res;
  for (const auto & new_region : response.splits().regions()) {
    if (new_region.id() == regionId()) {
      *state.mutable_region() = new_region;
    } else {
      auto region = new Region(new_region.id(), table_id);
      region->peer = *findPeerByStore(new_region, peer.store_id());
      region->apply_state.set_applied_index(5);
      *region->state.mutable_region() = new_region;
      data.split(region->data, new_region.start_key(), new_region.end_key());
      res.push_back(std::shared_ptr<Region>(region));
    }
  }
  return res;
}

void Region::mergeFrom(RegionData & src) {
  data.merge(src);
}

void Region::execAdminCmd(const raft_cmdpb::AdminRequest & request,
                          const raft_cmdpb::AdminResponse & response,
                          RaftCmdHeader header,
                          RegionManager & manager) {
  if (header.index <= appliedIndex()) {
    if (request.cmd_type() == raft_cmdpb::CommitMerge) {
      if (auto ori_region = manager.getRegion(request.commit_merge().source().id()); ori_region) {
        ori_region->state.set_state(raft_serverpb::PeerState::Tombstone);
        manager.eraseRegion(ori_region->regionId());
        manager.persist(*ori_region);
      }
    }
    return;
  }

  DEBUG(__FUNCTION__ << ": request: {" << request.ShortDebugString() << "}, response: {" << response.ShortDebugString()
                     << "}");
  RegionPtr ori_region = nullptr;
  switch (request.cmd_type()) {
    case raft_cmdpb::ChangePeer:
    case raft_cmdpb::ChangePeerV2: {
      *state.mutable_region() = response.change_peer().region();
      bool remove = !findPeerByStore(response.change_peer().region(), peer.store_id());
      if (remove)
        state.set_state(raft_serverpb::PeerState::Tombstone);
      break;
    }
    case raft_cmdpb::Split:
    case raft_cmdpb::BatchSplit: {
      for (const auto & region: splitInto(response)) {
        manager.insertRegion(region);
        manager.persist(*region);
      }
      break;
    }
    case raft_cmdpb::PrepareMerge: {
      *state.mutable_region() = response.split().left();
      state.set_state(raft_serverpb::PeerState::Merging);
      break;
    }
    case raft_cmdpb::CommitMerge: {
      *state.mutable_region() = response.split().left();
      ori_region = manager.getRegion(request.commit_merge().source().id());
      mergeFrom(ori_region->data);
      state.set_state(raft_serverpb::PeerState::Normal);
      ori_region->state.set_state(raft_serverpb::PeerState::Tombstone);
      break;
    }
    case raft_cmdpb::RollbackMerge: {
      *state.mutable_region() = response.split().left();
      state.set_state(raft_serverpb::PeerState::Normal);
      break;
    }
    default:break;
  }
  apply_state.set_applied_index(header.index);
  if (ori_region) {
    manager.eraseRegion(ori_region->regionId());
    manager.persist(*ori_region);
  }
  manager.persist(*this);
  if (isRemoving()) {
    manager.eraseRegion(regionId());
    DEBUG(__FUNCTION__ << ": remove " << debugInfo());
  }
}

DataBaseApplyRes HandleWriteRaftCmd(const DataBaseServer * server, WriteCmdsView cmds, RaftCmdHeader header) {
  auto & manager = server->context->getRegionManager();
  auto region = manager.getRegion(header.region_id);
  if (!region)
    return DataBaseApplyRes::NotFound;
  region->execWriteRaftCmd(cmds, header);
  return DataBaseApplyRes::None;
}

DataBaseApplyRes HandleAdminRaftCmd(const DataBaseServer * server,
                                    BaseBuffView req_buff,
                                    BaseBuffView resp_buff,
                                    RaftCmdHeader header) {

  raft_cmdpb::AdminRequest request;
  raft_cmdpb::AdminResponse response;
  request.ParseFromArray(req_buff.data, (int) req_buff.len);
  response.ParseFromArray(resp_buff.data, (int) resp_buff.len);
  auto & manager = server->context->getRegionManager();
  auto region = manager.getRegion(header.region_id);
  if (!region)
    return DataBaseApplyRes::NotFound;

  region->execAdminCmd(request, response, header, manager);

  return DataBaseApplyRes::Persist;
}

void AtomicUpdateProxy(DB::DataBaseServer * server, DB::DataBaseRaftProxyHelper * proxy) {
  server->proxy_helper = proxy;
}

void HandleDestroy(DataBaseServer * server, RegionId region_id) {
  INFO("destroy region " << region_id);
  auto & manager = server->context->getRegionManager();
  auto region = manager.getRegion(region_id);
  if (!region)
    return;
  region->setRemoving();
  manager.eraseRegion(region_id);
  manager.persist(*region);
}

DataBaseApplyRes HandleIngestSST(DataBaseServer *, SnapshotViewArray, RaftCmdHeader) {
  ERROR(__FUNCTION__ << ": not implemented");
  exit(-1);
}

uint8_t HandleCheckTerminated(DataBaseServer * server) {
  return server->context->getTerminal();
}

FsStats HandleComputeFsStats(DataBaseServer *) {
  FsStats res;
  res.capacity_size = 1024ull * 1024 * 1024 * 10;
  res.avail_size = 1024ull * 1024 * 1024 * 9;
  res.used_size = 1024ull * 1024 * 1024;
  res.ok = 1;
  return res;
}

DataBaseStatus HandleGetDataBaseStatus(DataBaseServer * server) { return server->status.load(); }

RaftProxyStatus DataBaseRaftProxyHelper::getProxyStatus() const {
  return static_cast<RaftProxyStatus>(fn_handle_get_proxy_status(proxy_ptr));
}
bool DataBaseRaftProxyHelper::checkEncryptionEnabled() const { return fn_is_encryption_enabled(proxy_ptr); }
EncryptionMethod DataBaseRaftProxyHelper::getEncryptionMethod() const { return fn_encryption_method(proxy_ptr); }
kvrpcpb::ReadIndexResponse DataBaseRaftProxyHelper::readIndex(const kvrpcpb::ReadIndexRequest & req) const {
  auto res = batchReadIndex({req});
  return std::move(res->at(0).first);
}
BatchReadIndexRes DataBaseRaftProxyHelper::batchReadIndex(const std::vector<kvrpcpb::ReadIndexRequest> & req) const {
  std::vector<std::string> req_strs;
  req_strs.reserve(req.size());
  for (auto & r : req) {
    req_strs.emplace_back(r.SerializeAsString());
  }
  CppStrVec data(std::move(req_strs));
  auto outer_view = data.intoOuterView();
  BatchReadIndexRes res(fn_handle_batch_read_index(proxy_ptr, outer_view));
  return res;
}

RegionPtr GenRegionPtr(metapb::Region && region, uint64_t peer_id, uint64_t index, uint64_t term) {
  auto region_id = region.id();
  auto peer = findPeer(region, peer_id);
  raft_serverpb::RaftApplyState apply_state;
  {
    apply_state.set_applied_index(index);
    apply_state.mutable_truncated_state()->set_index(index);
    apply_state.mutable_truncated_state()->set_term(term);
  }
  raft_serverpb::RegionLocalState state;
  *state.mutable_region() = std::move(region);
  return std::make_shared<Region>(region_id, std::move(*peer), std::move(apply_state), std::move(state));
}

RawCppPtr PreHandleTiKVSnapshot(
    DataBaseServer *,
    BaseBuffView region_buff,
    uint64_t peer_id,
    SnapshotViewArray snaps,
    uint64_t index,
    uint64_t term) {
  metapb::Region region;
  region.ParseFromArray(region_buff.data, (int) region_buff.len);
  auto new_region = GenRegionPtr(std::move(region), peer_id, index, term);

  size_t kv_size = 0;

  for (UInt64 i = 0; i < snaps.len; ++i) {
    auto & snapshot = snaps.views[i];
    assert(snapshot.cf == ColumnFamilyType::Write);
    for (UInt64 n = 0; n < snapshot.len; ++n) {
      auto & k = snapshot.keys[n];
      auto & v = snapshot.vals[n];
      new_region->insert(k, v);
    }
    kv_size += snapshot.len;
  }

  INFO(__FUNCTION__ << ": " << new_region->debugInfo(false) << " handle " << kv_size << " kv pairs");

  return RawCppPtr(new PreHandledTiKVSnapshot{new_region}, RawCppPtrType::PreHandledTiKVSnapshot);
}

void ApplyPreHandledSnapshot(DataBaseServer * server, void * res, RawCppPtrType type) {
  switch (type) {
    case RawCppPtrType::PreHandledTiKVSnapshot: {
      auto * snap = reinterpret_cast<PreHandledTiKVSnapshot *>(res);
      ApplyPreHandledTiKVSnapshot(server, snap);
      break;
    }
    case RawCppPtrType::PreHandledDataBaseSnapshot: {
      auto * snap = reinterpret_cast<PreHandledDataBaseSnapshot *>(res);
      ApplyPreHandledDataBaseSnapshot(server, snap);
      break;
    }
    default:throw Exception("ApplyPreHandledSnapshot unknown type " + std::to_string(uint32_t(type)));
  }
}

const char * IntoEncryptionMethodName(EncryptionMethod method) {
  static const char * EncryptionMethodName[] = {
      "Unknown",
      "Plaintext",
      "Aes128Ctr",
      "Aes192Ctr",
      "Aes256Ctr",
  };
  return EncryptionMethodName[static_cast<uint8_t>(method)];
}

BatchReadIndexRes::pointer GenBatchReadIndexRes(uint64_t cap) {
  auto res = new BatchReadIndexRes::element_type();
  res->reserve(cap);
  return res;
}

void InsertBatchReadIndexResp(BatchReadIndexRes::pointer resp, BaseBuffView view, uint64_t region_id) {
  kvrpcpb::ReadIndexResponse res;
  res.ParseFromArray(view.data, view.len);
  resp->emplace_back(std::move(res), region_id);
}

const char * RawCppPtrTypeName(RawCppPtrType type) {
  static const char * Names[] = {
      "None",
      "String",
      "PreHandledTiKVSnapshot",
      "DataBaseSnapshot",
      "PreHandledDataBaseSnapshot",
      "SplitKeys",
      "Unknown",
  };
  return Names[std::min(static_cast<size_t>(type), static_cast<size_t>(RawCppPtrType::Unknown))];
}

void GcRawCppPtr(DataBaseServer *, void * ptr, RawCppPtrType type) {
  if (ptr) {
    TRACE(__FUNCTION__ << ": raw cpp ptr " << RawCppPtrTypeName(type));

    switch (type) {
      case RawCppPtrType::String:delete reinterpret_cast<DataBaseRawString>(ptr);
        break;
      case RawCppPtrType::PreHandledTiKVSnapshot:delete reinterpret_cast<PreHandledTiKVSnapshot *>(ptr);
        break;
      case RawCppPtrType::DataBaseSnapshot: delete reinterpret_cast<DataBaseSnapshot *>(ptr);
        break;
      case RawCppPtrType::PreHandledDataBaseSnapshot: delete reinterpret_cast<PreHandledDataBaseSnapshot *>(ptr);
        break;
      case RawCppPtrType::SplitKeys: delete reinterpret_cast<SplitKeys *>(ptr);
        break;
      default:ERROR(__PRETTY_FUNCTION__ << ": unknown raw cpp ptr type " << static_cast<uint32_t>(type));
        exit(-1);
    }
  }
}

RawCppPtr GenDataBaseSnapshot(DataBaseServer * server, RaftCmdHeader header) {
  auto region = server->context->getRegionManager().getRegion(header.region_id);
  return RawCppPtr(new DataBaseSnapshot(RegionData(region->getData())), RawCppPtrType::DataBaseSnapshot);
}

SerializeDataBaseSnapshotRes SerializeDataBaseSnapshotInto(DataBaseServer *,
                                                           DataBaseSnapshot * snap,
                                                           BaseBuffView path) {
  return snap->serializeInto(path);
}

uint8_t IsDataBaseSnapshot(DataBaseServer *, BaseBuffView path_view) {
  std::string path(path_view);
  std::string s;
  s.resize(DataBaseSnapshot::SnapFlag.size());
  std::ifstream f(path);
  f.read(s.data(), DataBaseSnapshot::SnapFlag.size());
  if (f.good() && s == DataBaseSnapshot::SnapFlag)
    return 1;
  return 0;
}

void ApplyPreHandledDataBaseSnapshot(DataBaseServer * server, PreHandledDataBaseSnapshot * res) {
  INFO(__FUNCTION__ << ": " << res->region->debugInfo());
  server->context->getRegionManager().insertRegion(res->region);
  server->context->getRegionManager().persist(*res->region);
}

std::string Region::debugInfo(bool dump_status) const {
  std::stringstream ss;
  if (dump_status)
    ss << peer.ShortDebugString() << ", " << apply_state.ShortDebugString() << ", " << state.ShortDebugString();
  else
    ss << "[region " << regionId() << "]";
  return ss.str();
}

void ApplyPreHandledTiKVSnapshot(DataBaseServer * server, PreHandledTiKVSnapshot * res) {
  INFO(__FUNCTION__ << ": " << res->region->debugInfo());
  server->context->getRegionManager().insertRegion(res->region);
  server->context->getRegionManager().persist(*res->region);
}

void Region::deserializeData(const fs::path & path) {
  std::ifstream stream(path);
  stream.seekg(DataBaseSnapshot::SnapFlag.size());
  assert(stream.good());
  data.deserialize(stream);
  stream.close();
}

RawCppPtr PreHandleDataBaseSnapshot(DataBaseServer *,
                                    BaseBuffView region_buff,
                                    uint64_t peer_id,
                                    uint64_t index,
                                    uint64_t term,
                                    BaseBuffView path) {
  metapb::Region region;
  region.ParseFromArray(region_buff.data, (int) region_buff.len);
  auto new_region = GenRegionPtr(std::move(region), peer_id, index, term);
  new_region->deserializeData((std::string_view) path);
  return RawCppPtr(new PreHandledDataBaseSnapshot{new_region}, RawCppPtrType::PreHandledDataBaseSnapshot);
}

SerializeDataBaseSnapshotRes DataBaseSnapshot::serializeInto(std::string_view path_view) {
  std::string path(path_view);
  std::ofstream f(path);
  f.write(SnapFlag.data(), SnapFlag.size());
  assert(f.good());
  auto res = data.serialize(f);
  f.close();
  INFO("serialize snapshot {keys " << data.getKeys() << ", size " << res << "} into " << path);
  return {1, data.getKeys(), SnapFlag.size() + res};
}

GetRegionApproximateSizeKeysRes GetRegionApproximateSizeKeys(
    DataBaseServer * server, uint64_t region_id, BaseBuffView start_key, BaseBuffView end_key) {
  auto region = server->context->getRegionManager().getRegion(region_id);
  if (!region)
    return {0, 0, 0};
  DEBUG(__FUNCTION__ << ": " << region->debugInfo(0) << " size " << region->getSize() << ", keys "
                     << region->getKeys());
  return GetRegionApproximateSizeKeysRes{.ok = 1, .size = region->getSize(), .keys = region->getKeys()};
}

SplitKeysRes ScanSplitKeys(DataBaseServer * server,
                           uint64_t region_id,
                           BaseBuffView start_key,
                           BaseBuffView end_key,
                           CheckerConfig checker_config) {
  auto region = server->context->getRegionManager().getRegion(region_id);
  if (!region)
    return SplitKeysRes{0, 0, 0, SplitKeysWithView({})};

  DEBUG(__FUNCTION__ << ": region " << region_id << " ");

  if (checker_config.batch_split_limit == 0) {
    DEBUG("try to scan region and get half split key");
    auto split_keys = region->genHalfSplitKey();
    return SplitKeysRes{.ok = 1, .size = region->getSize(), .keys = region->getKeys(), .split_keys = SplitKeysWithView(
        split_keys ? std::vector<std::string>{std::move(*split_keys)} : std::vector<std::string>{})};
  }

  if (region->getSize() <= checker_config.max_size)
    return SplitKeysRes{1, 0, 0, SplitKeysWithView({})};

  auto split_keys = region->genHalfSplitKey();
  return SplitKeysRes{.ok = 1, .size = region->getSize(), .keys = region->getKeys(), .split_keys = SplitKeysWithView(
      split_keys ? std::vector<std::string>{std::move(*split_keys)} : std::vector<std::string>{})};
}

} // namespace DB
