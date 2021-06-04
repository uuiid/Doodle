//
// Created by TD on 2021/5/25.
//

#include <DoodleLib/Exception/Exception.h>
// clang-format off
#include <DoodleLib/Metadata/AssetsPath.h>
#include <DoodleLib/Metadata/Comment.h>

#include <DoodleLib/Metadata/Metadata.h>
#include <DoodleLib/Metadata/Assets.h>
#include <DoodleLib/Metadata/AssetsFile.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/Metadata/Shot.h>
// clang-format on
#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/core/ContainerDevice.h>
#include <DoodleLib/rpc/RpcClient.h>
#include <grpcpp/grpcpp.h>

#include <cereal/archives/portable_binary.hpp>

namespace doodle {

RpcClient::RpcClient(const std::shared_ptr<grpc::Channel>& in_channel)
    : p_stub(MetadataServer::NewStub(in_channel)),
      p_channel(in_channel) {
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
    //检查和验证
    if (
        k_prj->p_id != k_t.id())
      k_prj->p_id = k_t.id();
    k_out_list.emplace_back(std::dynamic_pointer_cast<Project>(k_prj));
  }

  return k_out_list;
}
std::vector<MetadataPtr> RpcClient::GetChild(const MetadataConstPtr& in_metadataPtr) {
  grpc::ClientContext k_context{};
  DataDb k_in_db{};
  DataVector k_out_db{};

  k_in_db.set_id(in_metadataPtr->getId());
  k_in_db.set_uuidpath(in_metadataPtr->getUrlUUID().generic_string());

  auto k_status = p_stub->GetChild(&k_context, k_in_db, &k_out_db);
  if (!k_status.ok()) {
    throw DoodleError{k_status.error_message()};
  }

  std::vector<MetadataPtr> list;
  for (const auto& k_i : k_out_db.data()) {
    MetadataPtr k_ptr;

    auto k_data = k_i.metadata_cereal().value();
    vector_container my_data{k_data.begin(), k_data.end()};
    {
      vector_istream k_i{my_data};
      cereal::PortableBinaryInputArchive k_archive{k_i};
      k_archive(k_ptr);
    }
    if (k_ptr->p_id == 0) {
      k_ptr->p_id = k_i.id();
    } else {
      if (k_ptr->p_id != k_i.id())
        continue;
    }

    list.emplace_back(k_ptr);
  }
  return list;
}
void RpcClient::GetMetadata(const MetadataPtr& in_metadataPtr) {
  grpc::ClientContext k_context{};
  DataDb k_in_db{};
  DataDb k_out_db{};

  k_in_db.set_id(in_metadataPtr->getId());
  k_in_db.set_uuidpath(in_metadataPtr->getUrlUUID().generic_string());

  auto k_status = p_stub->GetMetadata(&k_context, k_in_db, &k_out_db);
  if (!k_status.ok()) {
    throw DoodleError{k_status.error_message()};
  }

  auto k_data = k_out_db.metadata_cereal().value();
  vector_container my_data{k_data.begin(), k_data.end()};
  {
    MetadataPtr k_ptr;
    vector_istream k_i{my_data};
    cereal::PortableBinaryInputArchive k_archive{k_i};
    k_archive(k_ptr);
    in_metadataPtr->p_has_child = k_ptr->p_has_child;
  }

  in_metadataPtr->p_id = k_out_db.id();
}
void RpcClient::InstallMetadata(const MetadataPtr& in_metadataPtr) {
  if (in_metadataPtr->isInstall())
    return;

  grpc::ClientContext k_context{};
  DataDb k_in_db{};

  k_in_db.set_uuidpath(in_metadataPtr->getUrlUUID().generic_string());
  if (in_metadataPtr->hasParent())
    k_in_db.mutable_parent()->set_value(in_metadataPtr->p_parent_id.value());

  // #ifndef NDEBUG
  DOODLE_LOG_DEBUG(in_metadataPtr->str() << " has child " << (in_metadataPtr->hasChild() ? "true" : "false"));
  // #endif
  vector_container my_data{};
  {
    vector_iostream kt{my_data};
    cereal::PortableBinaryOutputArchive k_archive{kt};
    k_archive(in_metadataPtr);
  }

  k_in_db.mutable_metadata_cereal()->set_value(my_data.data(), my_data.size());

  DataDb k_out_db{};
  auto k_status = p_stub->InstallMetadata(&k_context, k_in_db, &k_out_db);
  if (k_status.ok()) {
    in_metadataPtr->p_id = k_out_db.id();
  } else {
    throw DoodleError{k_status.error_message()};
  }
}
void RpcClient::DeleteMetadata(const MetadataConstPtr& in_metadataPtr) {
}

void RpcClient::UpdataMetadata(const MetadataConstPtr& in_metadataPtr) {
  if (!in_metadataPtr->isInstall())
    return;

  grpc::ClientContext k_context{};
  DataDb k_in_db{};
  k_in_db.set_id(in_metadataPtr->getId());
  k_in_db.set_uuidpath(in_metadataPtr->getUrlUUID().generic_string());

  if (in_metadataPtr->hasParent())
    k_in_db.mutable_parent()->set_value(in_metadataPtr->p_parent_id.value());

  vector_container my_data{};
  {
    vector_iostream kt{my_data};
    cereal::PortableBinaryOutputArchive k_archive{kt};
    k_archive(in_metadataPtr);
  }

  k_in_db.mutable_metadata_cereal()->set_value(my_data.data(), my_data.size());

  DataDb k_out_db{};
  auto k_status = p_stub->UpdataMetadata(&k_context, k_in_db, &k_out_db);
  if (!k_status.ok()) {
    throw DoodleError{k_status.error_message()};
  }
}
}  // namespace doodle
