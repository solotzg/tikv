#pragma once

#include <iostream>
#include <sstream>
#include <mutex>

#include "Common.h"

namespace DB {
enum class LogType : uint8_t {
  Trace = 0,
  Debug,
  Info,
  Warn,
  Error,
};

const char * LogType2Name(LogType type);

struct LogOutput {
  void output(std::string && s) {
    std::lock_guard lock(mutex);
    std::cerr << s;
  }
  bool canOutput(LogType type) const {
    return type >= global_type;
  }
  LogOutput(LogType type) : global_type(type) {}
 private:
  const LogType global_type;
  std::mutex mutex;
};

inline void getTimestamp(std::stringstream & ss) {
  auto time_point = std::chrono::system_clock::now();
  auto tt = std::chrono::system_clock::to_time_t(time_point);

  std::tm * local_tm = std::localtime(&tt);
  int year = local_tm->tm_year + 1900;
  int month = local_tm->tm_mon + 1;
  int day = local_tm->tm_mday;
  int hour = local_tm->tm_hour;
  int minute = local_tm->tm_min;
  int second = local_tm->tm_sec;
  int milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count() % 1000;

  int zone_offset = local_tm->tm_gmtoff;

  char buffer[100] = "yyyy/MM/dd HH:mm:ss.SSS";

  std::sprintf(buffer, "%04d/%02d/%02d %02d:%02d:%02d.%03d", year, month, day, hour, minute, second, milliseconds);

  ss << buffer << " ";

  int offset_value = std::abs(zone_offset);
  auto offset_seconds = std::chrono::seconds(offset_value);
  auto offset_tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>(offset_seconds);
  auto offset_tt = std::chrono::system_clock::to_time_t(offset_tp);
  std::tm * offset_tm = std::gmtime(&offset_tt);
  if (zone_offset < 0)
    ss << "-";
  else
    ss << "+";
  char buff[] = "hh:mm";
  std::sprintf(buff, "%02d:%02d", offset_tm->tm_hour, offset_tm->tm_min);
  ss << buff;
}

extern LogOutput log_output;

#define BASE_LOG(__log_type, s) do{ if(log_output.canOutput(__log_type)) { std::stringstream __str_s; __str_s << "["; getTimestamp(__str_s); __str_s << "][DB-" << LogType2Name(__log_type) << "] " << s << '\n'; log_output.output(__str_s.str()); } } while (0)
#define TRACE(s) BASE_LOG(LogType::Trace, s)
#define DEBUG(s) BASE_LOG(LogType::Debug, s)
#define INFO(s) BASE_LOG(LogType::Info, s)
#define WARN(s) BASE_LOG(LogType::Warn, s)
#define ERROR(s) BASE_LOG(LogType::Error, s)
}