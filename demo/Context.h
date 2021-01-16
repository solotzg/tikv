#pragma once

#include "RegionManager.h"

namespace DB {
class GlobalContext : noncopyable {
 public:
  void stop() { terminal = true; }
  void init(std::string_view path, DataBaseRaftProxyHelper * proxy_helper_ = nullptr) {
    manager = std::make_unique<RegionManager>(path);
    proxy_helper = proxy_helper_;
  }
  bool getTerminal() const { return terminal; }
  RegionManager & getRegionManager() { return *manager; }
  DataBaseRaftProxyHelper * getProxyHelper() const { return proxy_helper; }
 private:
  std::atomic_bool terminal{false};
  std::unique_ptr<RegionManager> manager;
  DataBaseRaftProxyHelper * proxy_helper;
};
}