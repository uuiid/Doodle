//
// Created by TD on 2021/5/25.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <MetadataServer.grpc.pb.h.>

namespace doodle {
class RpcServer final : public MetadataServer::Service{

 public:
  RpcServer();
  ::grpc::Status GetProject(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::doodle::DataVector* response) override;
  ::grpc::Status GetChild(::grpc::ServerContext* context, const ::doodle::DataDb* request, ::doodle::DataVector* response) override;

  void runServer();
  void stop();
};
}
