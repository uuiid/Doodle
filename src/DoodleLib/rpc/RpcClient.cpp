//
// Created by TD on 2021/5/25.
//

#include <DoodleLib/rpc/RpcClient.h>
#include <Metadata/Metadata.h>
#include <core/Util.h>

#include <cereal/archives/portable_binary.hpp>

#include <grpcpp/grpcpp.h>
#include <Metadata/Project.h>

namespace doodle {

RpcClient::RpcClient( const std::shared_ptr<grpc::Channel>& in_channel)
    : p_stub(MetadataServer::NewStub(in_channel)) {
}
std::vector<ProjectPtr> RpcClient::GetProject() {
  grpc::ClientContext k_context{};
  DataVector k_vector;
  std::vector<ProjectPtr> k_list{};
  auto status = p_stub->GetProject(&k_context, {}, &k_vector);
  if (status.ok()) {
  }
  return std::vector<ProjectPtr>();
}
std::vector<MetadataPtr> RpcClient::GetChild(const MetadataConstPtr& in_metadataPtr) {
  return std::vector<MetadataPtr>();
}
void RpcClient::GetMetadata(const MetadataPtr& in_metadataPtr) {
}
void RpcClient::InstallMetadata(const MetadataPtr& in_metadataPtr) {
  grpc::ClientContext k_context{};
  DataDb k_in_db{};
  k_in_db.set_uuidpath("test");
  k_in_db.set_id(0);
//  k_in_db.set_uuidpath(in_metadataPtr->getUrlUUID().generic_string());
//
//  std::ostringstream k_stringstream{};
//  {
//    cereal::PortableBinaryOutputArchive k_archive{k_stringstream};
//    k_archive(in_metadataPtr);
//  }
//
//  const auto k_size = getStreamSize(k_stringstream);
//  k_in_db.mutable_metadata_cereal()->set_value(k_stringstream.rdbuf(),
//                                               k_size);
//  k_in_db.mutable_metadata_cereal()->set_value("test");
  DataDb k_out_db{};
  auto k_status = p_stub->GetMetadata(&k_context,k_in_db,&k_out_db);

  in_metadataPtr->p_id = k_out_db.id();
}
void RpcClient::DeleteMetadata(const MetadataConstPtr& in_metadataPtr) {
}
}  // namespace doodle
