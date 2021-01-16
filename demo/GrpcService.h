#pragma once

#include <memory>
#include <grpc++/grpc++.h>
#include <kvproto/tikvpb.grpc.pb.h>

#include "Common.h"

namespace DB {

class GlobalContext;
class FlashService final
    : public tikvpb::Tikv::Service, private noncopyable {
 public:
  ::grpc::Status RawGet(::grpc::ServerContext * context,
                        const ::kvrpcpb::RawGetRequest * request,
                        ::kvrpcpb::RawGetResponse * response) override;
  ::grpc::Status Coprocessor(::grpc::ServerContext * context,
                             const ::coprocessor::Request * request,
                             ::coprocessor::Response * response) override;

  explicit FlashService(GlobalContext * global_context_) : global_context(global_context_) {}
 private:
  GlobalContext * global_context;
};
}