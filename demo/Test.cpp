#include <kvproto/coprocessor.pb.h>
#include "ProxyFFI.h"
#include "Context.h"
#include "CopHelper.h"

namespace DB {

LogOutput log_output(LogType::Trace);
const std::string DataPathBase = "/tmp/__column_storage";
const std::string ProxyDataPath = DataPathBase + fs::path::preferred_separator + "proxy";
const std::string DataPath = DataPathBase + fs::path::preferred_separator + "col-store";
const std::string RegionDataPath = DataPath + fs::path::preferred_separator + "region-data";

extern RegionPtr GenRegionPtr(metapb::Region && region, uint64_t peer_id, uint64_t index, uint64_t term);

void TestRun() {
  GlobalContext global_context;
  global_context.init(DataPath);
  DataBaseServer database_instance_wrap{};
  database_instance_wrap.context = &global_context;
  auto & manager = global_context.getRegionManager();
  RegionId test_region_id = 12345;
  RegionId test_store_id = 3;
  {
    manager.eraseRegion(test_region_id);
    assert(manager.getRegion(test_region_id) == nullptr);
    metapb::Region region;
    region.set_id(test_region_id);
    region.set_start_key("10");
    region.set_end_key("19");
    auto epoch = region.mutable_region_epoch();
    epoch->set_version(2333);
    epoch->set_conf_ver(888);
    auto peer = region.add_peers();
    peer->set_id(region.id() + 1);
    peer->set_role(metapb::PeerRole::Voter);
    peer->set_store_id(test_store_id);
    std::vector<std::pair<std::string, std::string>> write_kv_list = {{"11", "12"}, {"12", "13"}};
    std::vector<BaseBuffView> keys;
    std::vector<BaseBuffView> vals;
    for (const auto & kv : write_kv_list) {
      keys.emplace_back(kv.first.data(), kv.first.size());
      vals.emplace_back(kv.second.data(), kv.second.size());
    }
    std::vector<SnapshotView> snaps;
    snaps.push_back(SnapshotView{keys.data(), vals.data(), ColumnFamilyType::Write, keys.size()});

    auto res = PreHandleTiKVSnapshot(&database_instance_wrap,
                                     region.SerializeAsString(),
                                     test_region_id + 1,
                                     SnapshotViewArray{snaps.data(), snaps.size()},
                                     20,
                                     10);
    ApplyPreHandledTiKVSnapshot(&database_instance_wrap, (PreHandledTiKVSnapshot *) res.ptr);
    GcRawCppPtr(nullptr, res.ptr, res.type);
  }
  auto region = manager.getRegion(test_region_id);

  {
    assert(region->getKeys() == 2);
    assert(region->getSize() == 8);
    region->insert("13", "2");
    region->insert("14", "3");
    region->insert("15", "4");
    region->insert("16", "5");
    assert(region->getKeys() == 6);
    assert(region->getSize() == 20);
    manager.persist(*region);
    GlobalContext context;
    context.init(DataPath);
    region = context.getRegionManager().getRegion(test_region_id);
    assert(region);
    assert(region->getKeys() == 6);
    assert(region->getSize() == 20);
    auto p = region->genHalfSplitKey();
    assert(p == "14");
    assert(region->isNormal());
    auto epoch = region->getEpoch();
    assert(epoch.version() == 2333);
    assert(epoch.conf_ver() == 888);
  }

  {
    std::vector<std::pair<std::string, std::string>> write_kv_list = {{"17", "18"}, {"18", "19"}};
    std::vector<BaseBuffView> keys;
    std::vector<BaseBuffView> vals;
    std::vector<WriteCmdType> cmd_types;
    std::vector<ColumnFamilyType> cmd_cf;

    for (const auto & kv : write_kv_list) {
      keys.emplace_back(kv.first.data(), kv.first.size());
      vals.emplace_back(kv.second.data(), kv.second.size());
      cmd_types.push_back(WriteCmdType::Put);
      cmd_cf.push_back(ColumnFamilyType::Write);
    }

    HandleWriteRaftCmd(&database_instance_wrap,
                       WriteCmdsView{.keys = keys.data(), .vals = vals.data(), .cmd_types = cmd_types.data(), .cmd_cf = cmd_cf.data(), .len = keys.size()},
                       RaftCmdHeader{test_region_id, 50, 11});
    region = manager.getRegion(test_region_id);
    assert(region->appliedIndex() == 50);
    assert(region->getSize() == 28);
    assert(region->getKeys() == 8);
  }

  {
    raft_cmdpb::AdminRequest request;
    raft_cmdpb::AdminResponse response;
    request.set_cmd_type(raft_cmdpb::AdminCmdType::BatchSplit);
    {
      metapb::Region new_region;
      new_region.set_id(test_region_id + 100);
      new_region.set_start_key("10");
      new_region.set_end_key("15");
      auto epoch = new_region.mutable_region_epoch();
      epoch->set_version(22);
      epoch->set_conf_ver(33);
      auto peer = new_region.add_peers();
      peer->set_id(new_region.id() + 1);
      peer->set_store_id(3);

      *response.mutable_splits()->add_regions() = std::move(new_region);
    }
    {
      metapb::Region new_region;
      new_region.set_id(test_region_id);
      new_region.set_start_key("15");
      new_region.set_end_key("30");
      auto epoch = new_region.mutable_region_epoch();
      epoch->set_version(52);
      epoch->set_conf_ver(63);
      auto peer = new_region.add_peers();
      peer->set_id(new_region.id() + 1);
      peer->set_store_id(3);

      *response.mutable_splits()->add_regions() = std::move(new_region);
    }
    HandleAdminRaftCmd(&database_instance_wrap,
                       request.SerializeAsString(),
                       response.SerializeAsString(),
                       RaftCmdHeader{test_region_id, 60, 11});

    auto src = manager.getRegion(test_region_id);
    auto tar = manager.getRegion(test_region_id + 100);
    assert(src->appliedIndex() == 60);
    assert(src->getLocalState().region().region_epoch().version() == 52);
    assert(src->getPeer().id() == test_region_id + 1);
    assert(tar->appliedIndex() == 5);
    assert(tar->getLocalState().region().region_epoch().version() == 22);
    assert(tar->getPeer().id() == test_region_id + 101);
    assert(src->getKeys() == 4);
    assert(tar->getKeys() == 4);
    assert(src->getSize() == 14);
    assert(tar->getSize() == 14);
    assert(tar->getData().data.begin()->first == "11");
    assert(src->getData().data.begin()->first == "15");
  }

  {
    auto region_meta = region->getLocalState().region();
    region->insert("20", "21");
    std::string snap_path = "/tmp/__test_snap.sst";
    {
      auto snap = GenDataBaseSnapshot(&database_instance_wrap, RaftCmdHeader{test_region_id, 1, 1});
      SerializeDataBaseSnapshotInto(&database_instance_wrap, (DataBaseSnapshot *) snap.ptr, snap_path);
      GcRawCppPtr(nullptr, snap.ptr, snap.type);
    }
    manager.eraseRegion(test_region_id);
    assert(IsDataBaseSnapshot(&database_instance_wrap, snap_path));
    {
      auto snap = PreHandleDataBaseSnapshot(&database_instance_wrap,
                                            region_meta.SerializeAsString(),
                                            test_region_id,
                                            777,
                                            666,
                                            snap_path);
      ApplyPreHandledDataBaseSnapshot(&database_instance_wrap, (PreHandledDataBaseSnapshot *) snap.ptr);
      GcRawCppPtr(nullptr, snap.ptr, snap.type);
    }
    assert(manager.getRegion(test_region_id)->getData().data.find("20")->second == "21");
    assert(manager.getRegion(test_region_id)->getSize() == 18);
  }

  {
    auto info = GetRegionApproximateSizeKeys(&database_instance_wrap, test_region_id, {nullptr, 0}, {nullptr, 0});
    assert(info.ok);
    assert(info.keys == manager.getRegion(test_region_id)->getKeys());
    assert(info.size == manager.getRegion(test_region_id)->getSize());
    auto res = ScanSplitKeys(&database_instance_wrap, test_region_id, {nullptr, 0}, {nullptr, 0}, {10, 5, 1});
    assert(res.split_keys.len == 1);
    assert(std::string_view(res.split_keys.view[0]) == "17");
    GcRawCppPtr(nullptr, res.split_keys.inner.ptr, res.split_keys.inner.type);
  }
  {
    metapb::Region new_region_meta;
    new_region_meta.set_id(test_region_id + 1001);
    auto peer = new_region_meta.add_peers();
    peer->set_id(new_region_meta.id() + 1);
    peer->set_store_id(3);
    auto new_region = GenRegionPtr(metapb::Region(new_region_meta), peer->id(), 888, 888);
    manager.insertRegion(new_region);
    raft_cmdpb::AdminRequest request;
    raft_cmdpb::AdminResponse response;
    request.set_cmd_type(raft_cmdpb::AdminCmdType::PrepareMerge);
    *response.mutable_split()->mutable_left() = new_region_meta;
    HandleAdminRaftCmd(&database_instance_wrap,
                       request.SerializeAsString(),
                       response.SerializeAsString(),
                       RaftCmdHeader{new_region_meta.id(), 889, 888});
    assert(manager.getRegion(new_region_meta.id())->getState() == raft_serverpb::PeerState::Merging);

    request = raft_cmdpb::AdminRequest();
    response = raft_cmdpb::AdminResponse();
    request.mutable_commit_merge()->mutable_source()->set_id(new_region_meta.id());
    request.set_cmd_type(raft_cmdpb::AdminCmdType::CommitMerge);
    *response.mutable_split()->mutable_left() = manager.getRegion(test_region_id)->getRegionMeta();
    HandleAdminRaftCmd(&database_instance_wrap,
                       request.SerializeAsString(),
                       response.SerializeAsString(),
                       RaftCmdHeader{test_region_id, 66, 66});
    assert(manager.getRegion(new_region_meta.id()) == nullptr);
  }
  {
    region = manager.getRegion(test_region_id);
    raft_cmdpb::AdminRequest request;
    raft_cmdpb::AdminResponse response;
    request.set_cmd_type(raft_cmdpb::AdminCmdType::ChangePeerV2);
    {
      auto new_region = new metapb::Region(region->getLocalState().region());
      auto epoch = new_region->mutable_region_epoch();
      epoch->set_conf_ver(44);
      new_region->clear_peers();
      response.mutable_change_peer_v2()->set_allocated_region(new_region);
    }
    HandleAdminRaftCmd(&database_instance_wrap,
                       request.SerializeAsString(),
                       response.SerializeAsString(),
                       RaftCmdHeader{test_region_id, 70, 11});
    assert(manager.getRegion(test_region_id) == nullptr);
    assert(region->isRemoving());
  }
}

void TestColTable() {
  {
    std::string val;
    auto task = TableColValueWrap(val);
    task.Add(99, 888);
    assert(TableColValueConstRef(val).Get(99) == 888);
    assert(TableColValueConstRef(val).Get(200) == 0);
    task.Add(99, 123);
    assert(TableColValueConstRef(val).Get(99) == (888+123));
  }

  {
    std::string buff(9, '\0');
    buff[0] = static_cast<char>(CopTaskType::Sum);
    *reinterpret_cast<int64_t *>(buff.data() + 1) = int64_t(98765);
    TableColCopTask task(buff);
    ::coprocessor::Response _ori_response;
    ::coprocessor::Response * response = &_ori_response;
    RegionData::DataType data = {{"123", "12"}, {"2", "321"}};
    std::string tar;
    bool ok = true;
    auto overloaded = variant_op::overloaded{
        [&response](Sum & sum) {
          DEBUG("run cop task sum: col_id " << sum.col_id);
          assert(sum.col_id == 98765);
          int64_t all = 67890;
          response->mutable_data()->resize(sizeof(all));
          *reinterpret_cast<int64_t *>(response->mutable_data()->data()) = all;
        },
        [&](HandleIDs & max) {
          max.encode(data, tar);
        },
        [&ok](auto && t) {
          ok = false;
        },
    };
    TRACE("size of overloaded is " << sizeof(overloaded));
    std::visit(overloaded, task.task);
    assert(response->data().size() == sizeof(int64_t));
    assert(*reinterpret_cast<const int64_t *>(response->mutable_data()->data()) == 67890);
    {
      buff = std::string(1, static_cast<char>(CopTaskType::HandleIDs));
      task = TableColCopTask(buff);
      std::visit(overloaded, task.task);
      std::string_view view = tar;
      uint64_t size = *reinterpret_cast<const uint64_t *>(view.data());
      assert(size == 2);
      view.remove_prefix(sizeof(uint64_t));
      std::set<std::string> _data;
      for (auto i = 0; i < size; ++i) {
        uint32_t sl = *reinterpret_cast<const uint32_t *>(view.data());
        view.remove_prefix(sizeof(uint32_t));
        {
          _data.emplace(view.substr(0, sl));
          view.remove_prefix(sl);
        }
      }
      assert(view.empty());
      {
        assert(_data.size() == data.size());
        for (auto & e : _data) {
          assert(data.find(e) != data.end());
        }
      }
    }
    {
      buff = std::string();
      task = TableColCopTask(buff);
      std::visit(overloaded, task.task);
      assert(!ok);
    }
  }
}

}

int main() {
  DB::TestRun();
  DB::TestColTable();
}