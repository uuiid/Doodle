//
// Created by TD on 2021/5/25.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <MetadataServer.grpc.pb.h>
#include <grpcpp/channel.h>

namespace doodle{
/**
 * @brief rpc客户端
 * @warning 这个类在导出的时候使用会报错, 在grpc库中会报空指针错误, 所有不可以在外部使用
 */
class DOODLELIB_API RpcClient {
  std::unique_ptr<MetadataServer::Stub> p_stub;
  std::shared_ptr<grpc::Channel> p_channel;
 public:
  explicit RpcClient( const std::shared_ptr<grpc::Channel>& in_channel);

  [[nodiscard]] std::vector<ProjectPtr> GetProject();
  [[nodiscard]] std::vector<MetadataPtr> GetChild(const MetadataConstPtr& in_metadataPtr);
  void GetMetadata(const MetadataPtr& in_metadataPtr);
  void InstallMetadata(const MetadataPtr& in_metadataPtr);
  void DeleteMetadata(const MetadataConstPtr& in_metadataPtr);
};
}
