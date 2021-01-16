#pragma once

#include "Common.h"
#include "Region.h"

namespace DB {

class RegionManager : noncopyable {
 public:
  RegionManager(std::string_view dir_path) : data_path(dir_path) {
    INFO("Create RegionManager under path:" << data_path);
    fs::path path(data_path);
    if (fs::exists(path)) {
      if (!fs::is_directory(path)) {
        ERROR("fail to restore all regions in path: " << data_path);
        exit(-1);
      }
      for (const auto & entry : fs::directory_iterator(path)) {
        auto filename = entry.path().filename();
        if (fs::is_directory(entry.status())) {
          ERROR("fail to restore data about region " << filename << " in path: " << entry.path());
          exit(-1);
        }
        DEBUG("Try to init region under path: " << entry.path());
        auto region = std::make_shared<Region>(entry);
        regions.emplace(region->regionId(), std::move(region));
      }
    }
    INFO("restore " << regions.size() << " regions");
  }
  RegionPtr getRegion(RegionId id) const {
    if (auto it = regions.find(id); it != regions.end())
      return it->second;
    return nullptr;
  }
  void eraseRegion(RegionId id) {
    regions.erase(id);
  }
  void insertRegion(RegionPtr region) { regions.emplace(region->regionId(), region); }

  void persist(Region & region) {
    auto region_path = data_path + fs::path::preferred_separator + std::to_string(region.regionId());
    if (region.isRemoving()) {
      fs::remove(region_path);
    } else {
      fs::create_directories(data_path);
      region.serialize(region_path);
    }
  }

 private:
  std::unordered_map<RegionId, RegionPtr> regions;
  std::string data_path;
};

}