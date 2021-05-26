//
// Created by TD on 2021/5/25.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/core/CoreSet.h>

#include <MetadataServer.grpc.pb.h.>


namespace doodle {
class DOODLELIB_API RpcServer final : public MetadataServer::Service{
  CoreSet& p_set;
  static std::unique_ptr<grpc::Server> p_Server;

  [[nodiscard]] inline FSys::path getPath(const std::string & in_string) const;
//  [[nodiscard]] FSys::path getPath(uint64_t id,const std::string& in_string)const;
 public:
  RpcServer();
  grpc::Status GetProject(grpc::ServerContext* context, const google::protobuf::Empty* request, DataVector* response) override;
  grpc::Status GetChild(grpc::ServerContext* context, const DataDb* request, DataVector* response) override;
  grpc::Status GetMetadata(grpc::ServerContext* context, const DataDb* request, DataDb* response) override;
  grpc::Status InstallMetadata(grpc::ServerContext* context, const DataDb* request, DataDb* response) override;
  grpc::Status DeleteMetadata(grpc::ServerContext* context, const DataDb* request, DataDb* response) override;

  DOODLE_DISABLE_COPY(RpcServer)

  static void runServer();
  static void stop();
};
}
