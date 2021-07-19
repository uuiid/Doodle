//
// Created by TD on 2021/5/25.
//

#include "RpcMetadataClient.h"

#include <DoodleLib/Exception/Exception.h>
// clang-format off
#include <DoodleLib/Metadata/Metadata_cpp.h>
// clang-format on
#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/core/ContainerDevice.h>
#include <grpcpp/grpcpp.h>

#include <cereal/archives/portable_binary.hpp>

namespace doodle {

RpcMetadataClient::RpcMetadataClient(const std::shared_ptr<grpc::Channel>& in_channel)
    : p_stub(MetadataServer::NewStub(in_channel))
// p_channel(in_channel)
{
  //  auto k_s = p_channel->GetState(true);
}
std::vector<ProjectPtr> RpcMetadataClient::GetProject() {
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

    if (k_data.empty())
      throw DoodleError{"无法读取项目详细信息"};

    vector_container my_data{k_data.begin(), k_data.end()};
    {
      vector_istream k_istream{my_data};
      if (!k_istream.good())
        throw DoodleError{"打开文件失败"};
      cereal::PortableBinaryInputArchive ar{k_istream};
      ar(k_prj);
    }
    //检查和验证
    if (k_prj->p_id != k_t.id())
      k_prj->p_id = k_t.id();

    k_prj->set_meta_type(magic_enum::enum_integer(k_t.m_type()));

    k_out_list.emplace_back(std::dynamic_pointer_cast<Project>(k_prj));
  }

  return k_out_list;
}
std::vector<MetadataPtr> RpcMetadataClient::GetChild(const MetadataConstPtr& in_metadataPtr) {
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
      vector_istream k_v{my_data};
      cereal::PortableBinaryInputArchive k_archive{k_v};
      k_archive(k_ptr);
    }
    if (k_ptr->p_id == 0) {
      k_ptr->p_id = k_i.id();
    } else {
      if (k_ptr->p_id != k_i.id())
        continue;
    }
    k_ptr->set_meta_type(magic_enum::enum_integer(k_i.m_type()));
    list.emplace_back(k_ptr);
  }
  return list;
}
void RpcMetadataClient::GetMetadata(const MetadataPtr& in_metadataPtr) {
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
  in_metadataPtr->set_meta_type(magic_enum::enum_integer(k_out_db.m_type()));
  in_metadataPtr->p_id = k_out_db.id();
}
void RpcMetadataClient::InstallMetadata(const MetadataPtr& in_metadataPtr) {
  if (in_metadataPtr->isInstall())
    return;

  grpc::ClientContext k_context{};
  DataDb k_in_db{};

  k_in_db.set_uuidpath(in_metadataPtr->getUrlUUID().generic_string());
  k_in_db.set_m_type(magic_enum::enum_cast<doodle::DataDb::meta_type>(in_metadataPtr->get_meta_type_int()).value());
  if (in_metadataPtr->hasParent())
    k_in_db.mutable_parent()->set_value(in_metadataPtr->p_parent_id.value());

  // #ifndef NDEBUG
  DOODLE_LOG_DEBUG(fmt::format("{} 子物体 -> {} ", in_metadataPtr->str(), in_metadataPtr->hasChild()));
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
void RpcMetadataClient::DeleteMetadata(const MetadataConstPtr& in_metadataPtr) {
  if (!in_metadataPtr->isInstall())
    return;

  grpc::ClientContext k_context{};
  DataDb k_in_db{};
  k_in_db.set_id(in_metadataPtr->getId());
  k_in_db.set_uuidpath(in_metadataPtr->getUrlUUID().generic_string());

  DataDb k_out_db{};
  auto k_status = p_stub->DeleteMetadata(&k_context, k_in_db, &k_out_db);
  DOODLE_LOG_WARN("删除数据 : {} 路径 {}", in_metadataPtr->getId(), in_metadataPtr->getUrlUUID())
  if (!k_status.ok()) {
    throw DoodleError{k_status.error_message()};
  }
}

void RpcMetadataClient::UpdateMetadata(const MetadataConstPtr& in_metadataPtr) {
  UpdateMetadata(in_metadataPtr, false);
}

void RpcMetadataClient::UpdateMetadata(const MetadataConstPtr& in_metadataPtr, bool b_update_parent_id) {
  if (!in_metadataPtr->isInstall())
    return;

  grpc::ClientContext k_context{};
  DataDb k_in_db{};
  k_in_db.set_id(in_metadataPtr->getId());
  k_in_db.set_uuidpath(in_metadataPtr->getUrlUUID().generic_string());

  if (in_metadataPtr->hasParent() && b_update_parent_id)
    k_in_db.mutable_parent()->set_value(in_metadataPtr->p_parent_id.value());

  vector_container my_data{};
  {
    vector_iostream kt{my_data};
    cereal::PortableBinaryOutputArchive k_archive{kt};
    k_archive(in_metadataPtr);
  }

  k_in_db.mutable_metadata_cereal()->set_value(my_data.data(), my_data.size());

  DataDb k_out_db{};
  auto k_status = p_stub->UpdateMetadata(&k_context, k_in_db, &k_out_db);
  if (!k_status.ok()) {
    throw DoodleError{k_status.error_message()};
  }
}
}  // namespace doodle
