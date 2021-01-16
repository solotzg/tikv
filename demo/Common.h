#pragma once

#include <fstream>

#ifdef __GNUC__
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include "Log.h"

namespace noncopyable_  // protection from unintended ADL
{
class noncopyable {
 protected:
  noncopyable() = default;
 private:
  noncopyable(const noncopyable &) = delete;
  noncopyable & operator=(const noncopyable &) = delete;
};
}

typedef noncopyable_::noncopyable noncopyable;

namespace variant_op {
template<class... Ts>
struct overloaded : Ts ... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
}

namespace DB {
template<typename T>
inline size_t WriteStreamInt(std::ofstream & stream, const T & s) {
  stream.write(reinterpret_cast<const char *>(&s), sizeof(s));
  return sizeof(s);
}
template<typename T>
inline void ReadStreamInt(std::ifstream & stream, T & s) {
  stream.read(reinterpret_cast<char *>(&s), sizeof(s));
}

inline size_t WriteStreamStr(std::ofstream & stream, std::string_view str) {
  auto res = 0;
  res += WriteStreamInt(stream, str.size());
  stream.write(str.data(), str.size());
  res += str.size();
  return res;
}

inline void ReadStreamStr(std::ifstream & stream, std::string & str) {
  size_t size;
  ReadStreamInt(stream, size);
  str.resize(size);
  stream.read(str.data(), size);
}

using RegionId = uint64_t;
using AppliedIndex = uint64_t;
}
