#pragma once

#include <kvproto/metapb.pb.h>
#include <kvproto/raft_serverpb.pb.h>
#include <kvproto/raft_cmdpb.pb.h>

#include "Common.h"
#include "ProxyFFI.h"
#include "Codec.h"

namespace DB {

class Region;
class RegionManager;
using RegionPtr = std::shared_ptr<Region>;

struct RegionData {
  using DataType = std::map<std::string, std::string>;

  size_t serialize(std::ofstream & stream) const {
    auto res = 0;
    size_t cnt = data.size();
    res += WriteStreamInt(stream, cnt);
    for (const auto &[k, v]: data) {
      res += WriteStreamStr(stream, k);
      res += WriteStreamStr(stream, v);
    }
    return res;
  }

  void deserialize(std::ifstream & stream) {
    size_t cnt = 0;
    ReadStreamInt(stream, cnt);
    for (size_t i = 0; i < cnt; ++i) {
      std::string k, v;
      ReadStreamStr(stream, k);
      ReadStreamStr(stream, v);
      size += k.size();
      size += v.size();
      data.emplace(std::move(k), std::move(v));
    }
  }

  DataType::iterator insert(std::string_view key, std::string_view val) {
    auto[it, ok] = data.emplace(key, val);
    if (ok) {
      size += key.size();
      size += val.size();
    } else {
      size -= it->second.size();
      it->second = val;
      size += it->second.size();
    }
    return it;
  }
  void remove(const std::string & key) {
    remove(data.find(key));
  }

  size_t getSize() const { return size; }
  size_t getKeys() const { return data.size(); }

  void split(RegionData & new_data, std::string_view start_key, std::string_view end_key) {
    for (auto it = data.begin(); it != data.end();) {
      if (it->first.compare(start_key) >= 0 && it->first.compare(end_key) < 0) {
        new_data.insert(it->first, it->second);
        it = remove(it);
      } else {
        ++it;
      }
    }
  }

  void merge(RegionData & src) {
    for (const auto & e : src.data) {
      insert(e.first, e.second);
    }
  }

  std::optional<std::string> genHalfSplitKey() const {
    size_t cnt = 0;
    for (const auto &[k, _] : data) {
      ++cnt;
      if (cnt > data.size() / 2)
        return k;
    }
    return std::nullopt;
  }

  DataType data;
  size_t size{0};

 private:
  decltype(data)::iterator remove(decltype(data)::iterator it) {
    if (it != data.end()) {
      size -= it->first.size();
      size -= it->second.size();
      return data.erase(it);
    }
    return data.end();
  }
};

inline bool SerializeToOstream(const google::protobuf::Message & msg, std::ofstream & stream) {
  auto tmp = msg.SerializeAsString();
  return WriteStreamStr(stream, tmp);
}

inline bool ParseFromIstream(google::protobuf::Message & msg, std::ifstream & stream) {
  std::string str;
  ReadStreamStr(stream, str);
  return msg.ParseFromString(str);
}

class Region : noncopyable {
 public:
  static const TableId SchemaColTableID;
  explicit Region(const fs::path & path) : id(atoi(path.filename().c_str())) {
    DEBUG("Start to init region " << id);
    deserialize(path);
    INFO("Restored " << state.ShortDebugString() << " " << apply_state.ShortDebugString());
    try {
      table_id = GetTableIdRaw(DecodeTikvKey(state.region().start_key()));
    } catch (...) {
      ERROR("fail to get table id of " << debugInfo(false));
    }
  }

  RegionId regionId() const { return id; }
  void serialize(const fs::path & path) const {
    std::ofstream stream(path);
    SerializeToOstream(peer, stream);
    SerializeToOstream(apply_state, stream);
    SerializeToOstream(state, stream);
    data.serialize(stream);
    stream.close();
  }

  void deserialize(const fs::path & path) {
    std::ifstream stream(path);
    ParseFromIstream(peer, stream);
    ParseFromIstream(apply_state, stream);
    ParseFromIstream(state, stream);
    data.deserialize(stream);
    stream.close();
  }

  void deserializeData(const fs::path & path);

  void execWriteRaftCmd(const WriteCmdsView & cmds, RaftCmdHeader header);

  void execAdminCmd(const raft_cmdpb::AdminRequest & request,
                    const raft_cmdpb::AdminResponse & response,
                    RaftCmdHeader header,
                    RegionManager & manager);

  Region(RegionId id_,
         metapb::Peer peer_,
         raft_serverpb::RaftApplyState apply_state_,
         raft_serverpb::RegionLocalState state_)
      : id(id_),
        peer(std::move(peer_)),
        apply_state(std::move(apply_state_)),
        state(std::move(state_)) {
    try {
      table_id = GetTableIdRaw(DecodeTikvKey(state.region().start_key()));
    } catch (...) {
      ERROR("fail to get table id of " << debugInfo(false));
    }
  }

  std::string debugInfo(bool dump_status = true) const;
  void insert(std::string_view k, std::string_view v) {
    data.insert(k, v);
  }

  size_t getSize() const { return data.getSize(); }
  size_t getKeys() const { return data.getKeys(); }

  std::optional<std::string> genHalfSplitKey() const { return data.genHalfSplitKey(); }

  const RegionData & getData() const { return data; }
  const metapb::RegionEpoch & getEpoch() const { return state.region().region_epoch(); }
  const metapb::Region & getRegionMeta() const { return state.region(); }
  bool isNormal() const { return state.state() == raft_serverpb::PeerState::Normal; }
  bool isRemoving() const { return state.state() == raft_serverpb::PeerState::Tombstone; }
  ::raft_serverpb::PeerState getState() const { return state.state(); }
  explicit Region(const RegionId id_, const TableId table_id_) : id(id_), table_id(table_id_) {}
  uint64_t appliedIndex() const { return apply_state.applied_index(); }
  raft_serverpb::RegionLocalState getLocalState() const { return state; }
  metapb::Peer getPeer() const { return peer; }
  void setRemoving() { state.set_state(raft_serverpb::PeerState::Tombstone); }

 private:
  std::vector<RegionPtr> splitInto(const raft_cmdpb::AdminResponse & response);
  void mergeFrom(RegionData & data);
 private:
  const RegionId id;
  metapb::Peer peer;
  raft_serverpb::RaftApplyState apply_state;
  raft_serverpb::RegionLocalState state;
  RegionData data;
  TableId table_id{0};
};
}