//
// Created by TD on 2021/5/25.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <MetadataServer.grpc.pb.h.>
#include <DoodleLib/core/CoreSet.h>

namespace doodle {
class RpcServer final : public MetadataServer::Service{
  CoreSet& p_set;


  [[nodiscard]] FSys::path getPath(const std::string & in_string) const;
//  [[nodiscard]] FSys::path getPath(uint64_t id,const std::string& in_string)const;
 public:
  RpcServer();
  ::grpc::Status GetProject(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::doodle::DataVector* response) override;
  ::grpc::Status GetChild(::grpc::ServerContext* context, const ::doodle::DataDb* request, ::doodle::DataVector* response) override;

  void runServer();
  void stop();
};
}
