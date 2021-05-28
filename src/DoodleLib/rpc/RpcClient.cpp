//
// Created by TD on 2021/5/25.
//

#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/core/ContainerDevice.h>
#include <DoodleLib/rpc/RpcClient.h>
#include <DoodleLib/Metadata/Metadata.h>
#include <DoodleLib/Metadata/Project.h>

#include <grpcpp/grpcpp.h>

#include <cereal/archives/portable_binary.hpp>

namespace doodle {

RpcClient::RpcClient(const std::shared_ptr<grpc::Channel>& in_channel)
    : p_stub(MetadataServer::NewStub(in_channel)),
      p_channel(in_channel){
//  auto k_s = p_channel->GetState(true);

}
std::vector<ProjectPtr> RpcClient::GetProject() {
  grpc::ClientContext k_context{};
  DataVector k_vector;
  std::vector<ProjectPtr> k_out_list{};
  auto status = p_stub->GetProject(&k_context, {}, &k_vector);
  if (!status.ok())
    throw DoodleError{status.error_message()};

  /// 转换缓冲区类型和向量类型
  for (const auto& k_t : k_vector.data()) {
    MetadataPtr k_prj{};
    const auto k_data = k_t.metadata_cereal().value();

    vector_container my_data{k_data.begin(), k_data.end()};
    {
      vector_istream k_istream{my_data};
      if (!k_istream.good())
        throw DoodleError{"打开文件失败"};
      cereal::PortableBinaryInputArchive ar{k_istream};
      ar(k_prj);
    }

    if (
        k_prj->p_id != k_t.id())
      continue;
    k_out_list.emplace_back(std::dynamic_pointer_cast<Project>(k_prj));
  }

  return k_out_list;
}
std::vector<MetadataPtr> RpcClient::GetChild(const MetadataConstPtr& in_metadataPtr) {
  return std::vector<MetadataPtr>();
}
void RpcClient::GetMetadata(const MetadataPtr& in_metadataPtr) {
}
void RpcClient::InstallMetadata(const MetadataPtr& in_metadataPtr) {
    grpc::ClientContext k_context{};
    DataDb k_in_db{};
    k_in_db.set_uuidpath(in_metadataPtr->getUrlUUID().generic_string());

    vector_container my_data{};
    {
      vector_iostream kt{my_data};
      cereal::PortableBinaryOutputArchive k_archive{kt};
      k_archive(in_metadataPtr);
    }

    k_in_db.mutable_metadata_cereal()->set_value(my_data.data(),my_data.size());

    DataDb k_out_db{};
    auto k_status = p_stub->InstallMetadata(&k_context,k_in_db,&k_out_db);
    if(k_status.ok()){
      in_metadataPtr->p_id = k_out_db.id();
    } else{
      throw DoodleError{k_status.error_message()};
    }
//  auto path = FSys::path{"D:/Doodle_cache"} / in_metadataPtr->getUrlUUID();
//  if (!FSys::exists(path.parent_path()))
//    FSys::create_directories(path.parent_path());
//  {
//    FSys::ofstream k_fstream{path, std::ios::binary | std::ios::out};
//    cereal::PortableBinaryOutputArchive k_archive{k_fstream};
//    k_archive(in_metadataPtr);
//  }
//  {
//    FSys::ifstream k_ifstream{path, std::ios::binary | std::ios::in};
//    cereal::PortableBinaryInputArchive k_archive{k_ifstream};
//    ProjectPtr k_ptr;
//    k_archive(k_ptr);
//  }
}
void RpcClient::DeleteMetadata(const MetadataConstPtr& in_metadataPtr) {
}
}  // namespace doodle
