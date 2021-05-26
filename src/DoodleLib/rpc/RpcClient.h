//
// Created by TD on 2021/5/25.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <MetadataServer.grpc.pb.h>
#include <grpcpp/channel.h>

namespace doodle{
class DOODLELIB_API RpcClient {
  std::unique_ptr<MetadataServer::Stub> p_stub;

 public:
  explicit RpcClient(const std::shared_ptr<grpc::Channel>& in_channel);

  [[nodiscard]] std::vector<ProjectPtr> GetProject();
  [[nodiscard]] std::vector<MetadataPtr> GetChild(const MetadataPtr& in_metadataPtr);
  void GetMetadata(const MetadataPtr& in_metadataPtr);
  void InstallMetadata(const MetadataPtr& in_metadataPtr);
  void DeleteMetadata(const MetadataConstPtr& in_metadataPtr);
};
}
