#include "Common.h"

#include <variant>

namespace DB {

enum class EngineModifyType : uint8_t {
  Del = 1,
  Add,
};

struct TableColValueWrap {
  std::string & val;
  uint64_t size;
  int64_t * data;

  void reset() {
    if (val.empty()) {
      size = 0;
      data = nullptr;
      return;
    }
    size = *reinterpret_cast<const uint64_t *>(val.data());
    data = reinterpret_cast<int64_t *>(val.data() + sizeof(size));
  }

  explicit TableColValueWrap(std::string & val_) : val(val_) {
    reset();
  }

  ssize_t initCol(size_t col_num) {
    if (col_num > size) {
      auto ori_size = val.size();
      size = col_num;
      val.resize(sizeof(uint64_t) + sizeof(int64_t) * col_num, 0);
      *reinterpret_cast<uint64_t *>(val.data()) = size;
      data = reinterpret_cast<int64_t *>(val.data() + sizeof(uint64_t));
      return val.size() - ori_size;
    }
    return 0;
  }

  ssize_t Add(int64_t col_id, int64_t d) {
    auto delta = initCol(col_id + 1);
    data[col_id] += d;
    return delta;
  }
};

struct TableColValueConstRef {
  std::string_view val;
  uint64_t size;
  const int64_t * data;

  TableColValueConstRef(std::string_view val_) : val(val_) {
    if (val.empty()) {
      size = 0;
      data = nullptr;
    } else {
      size = *reinterpret_cast<const uint64_t *>(val.data());
      data = reinterpret_cast<const int64_t *>(val.data() + sizeof(uint64_t));
    }
  }
  int64_t Get(int64_t col_id) const {
    if (col_id < size)
      return data[col_id];
    return 0;
  }
};

struct Sum {
  int64_t col_id;
  Sum(std::string_view slice) {
    col_id = *reinterpret_cast<const int64_t *>(slice.data());
  }
};

struct HandleIDs {
  void encode(const RegionData::DataType & region_data, std::string & tar) const {
    std::stringstream ss;
    uint64_t size = region_data.size();
    ss.write(reinterpret_cast<const char *>(&size), sizeof(size));
    for (auto & d : region_data) {
      uint32_t s = d.first.size();
      ss.write(reinterpret_cast<const char *>(&s), sizeof(s));
      ss.write(d.first.data(), s);
    }
    tar = ss.str();
  }
};

struct KillSelf {
};

enum class CopTaskType : uint8_t {
  Sum = 1,
  HandleIDs,
  KillSelf,
};

struct TableColCopTask {
  std::string_view buff;
  std::variant<std::monostate, Sum, HandleIDs, KillSelf> task;
  TableColCopTask(std::string_view buff_) : buff(buff_) {
    if (buff.empty())
      return;
    auto type = static_cast<CopTaskType>(buff[0]);
    switch (type) {
      case CopTaskType::Sum: {
        task = Sum(buff.substr(1, sizeof(int64_t)));
        break;
      }
      case CopTaskType::HandleIDs: {
        task = HandleIDs();
        break;
      }
      case CopTaskType::KillSelf: {
        task = KillSelf{};
        break;
      }
    }
  }
  TableColCopTask(const TableColCopTask &) = delete;
};

}