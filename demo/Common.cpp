#include "Common.h"

namespace DB {

const char * LogType2Name(LogType type) {
  static const char * TYPE_NAMES[] = {
      "Trace",
      "Debug",
      "Info ",
      "Warn ",
      "Error",
  };
  return TYPE_NAMES[size_t(type)];
}
}