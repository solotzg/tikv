#include <thread>
#include <csignal>

#include "GrpcService.h"
#include "Context.h"

#include "CopHelper.h"

namespace DB {

std::lock_guard<std::mutex> GenGlobalLock();

kvrpcpb::ReadIndexRequest GenRegionReadIndexReq(const Region & region) {
  kvrpcpb::ReadIndexRequest request;
  {
    auto context = request.mutable_context();
    context->set_region_id(region.regionId());
    *context->mutable_peer() = region.getPeer();
    context->mutable_region_epoch()->set_version(region.getEpoch().version());
    context->mutable_region_epoch()->set_conf_ver(region.getEpoch().conf_ver());
  }
  return request;
}

extern std::sig_atomic_t SignalStatus;

::grpc::Status FlashService::Coprocessor(::grpc::ServerContext * context,
                                         const ::coprocessor::Request * request,
                                         ::coprocessor::Response * response) {
  auto region_id = request->context().region_id();
  RegionPtr region = nullptr;
  kvrpcpb::ReadIndexRequest read_index_request;
  {
    auto _lock = GenGlobalLock();
    auto region_epoch = request->context().region_epoch();
    region = global_context->getRegionManager().getRegion(region_id);
    if (!region || !region->isNormal() || global_context->getTerminal()) {
      response->mutable_region_error()->mutable_region_not_found()->set_region_id(region_id);
      return ::grpc::Status::OK;
    } else if (auto cur_epoch = region->getEpoch(); cur_epoch.version() != region_epoch.version()) {
      response->mutable_region_error()->mutable_region_not_found()->set_region_id(region_id);
      return ::grpc::Status::OK;
    }
    read_index_request = GenRegionReadIndexReq(*region);
  }
  {
    auto proxy_helper = global_context->getProxyHelper();
    if (proxy_helper) {
      kvrpcpb::ReadIndexRequest req;
      auto resp = proxy_helper->batchReadIndex({read_index_request});
      assert(resp->size() == 1);

      {
        auto _lock = GenGlobalLock();
        if (auto & e = resp->at(0).first; !e.has_region_error()) {
          DEBUG("region " << region_id << " with current apply index " << region->appliedIndex()
                          << " , need to wait index " << e.read_index());
          if (e.read_index() < region->appliedIndex()) {
            ERROR("TODO: need to implement async wait index, this demo do not contain related function");
            response->mutable_region_error()->mutable_region_not_found()->set_region_id(region_id);
            return ::grpc::Status::OK;
          }
        } else {
          ERROR("got region error: " << resp->at(0).first.region_error().ShortDebugString());
          response->mutable_region_error()->mutable_region_not_found()->set_region_id(region_id);
          return ::grpc::Status::OK;
        }
      }
    }
  }
  {
    auto _lock = GenGlobalLock();
    TableColCopTask task(request->data());
    bool ok = true;
    std::visit(variant_op::overloaded{
        [&region, &response](Sum & sum) {
          int64_t all = 0;
          for (auto & d : region->getData().data) {
            all += TableColValueConstRef(d.second).Get(sum.col_id);
          }
          response->mutable_data()->resize(sizeof(all));
          *reinterpret_cast<int64_t *>(response->mutable_data()->data()) = all;
        },
        [&region, &response](HandleIDs & max) {
          max.encode(region->getData().data, *response->mutable_data());
        },
        [&response](KillSelf & k) {
          constexpr int64_t magic = 654321;
          auto & s = *response->mutable_data();
          s.resize(sizeof(magic));
          *reinterpret_cast<int64_t *>(s.data()) = magic;
          INFO("Got a kill message and try to kill -9 after a while");
          SignalStatus = 543210;
        },
        [&ok](auto && t) {
          ok = false;
        },
    }, task.task);
    if (!ok) {
      return ::grpc::Status(::grpc::StatusCode::INTERNAL, "unsupported cop task");
    }
  }
  {
    // check again after read.
    auto _lock = GenGlobalLock();
    auto region_epoch = request->context().region_epoch();
    if (global_context->getRegionManager().getRegion(region_id) != region) {
      response->mutable_region_error()->mutable_region_not_found()->set_region_id(region_id);
    } else {
      if (!region || !region->isNormal() || global_context->getTerminal()) {
        response->mutable_region_error()->mutable_region_not_found()->set_region_id(region_id);
      } else if (auto cur_epoch = region->getEpoch(); cur_epoch.version() != region_epoch.version()) {
        response->mutable_region_error()->mutable_region_not_found()->set_region_id(region_id);
      }
    }
  }
  return ::grpc::Status::OK;
}

::grpc::Status FlashService::RawGet(::grpc::ServerContext * context,
                                    const ::kvrpcpb::RawGetRequest * request,
                                    ::kvrpcpb::RawGetResponse * response) {
  auto _lock = GenGlobalLock();
  auto region_id = request->context().region_id();
  auto region_epoch = request->context().region_epoch();
  auto region = global_context->getRegionManager().getRegion(region_id);
  if (!region || !region->isNormal() || global_context->getTerminal()) {
    response->mutable_region_error()->mutable_region_not_found()->set_region_id(region_id);
  } else if (auto cur_epoch = region->getEpoch(); cur_epoch.version() != region_epoch.version()) {
    response->mutable_region_error()->mutable_epoch_not_match();
  } else {
    if (auto it = region->getData().data.find(request->key()); it != region->getData().data.end()) {
      response->set_value(it->second);
    }
  }
  return ::grpc::Status::OK;
}
}