﻿//
// Created by TD on 2021/5/25.
//

#include "RpcMetadataClient.h"

#include <DoodleLib/Exception/Exception.h>
// clang-format off
#include <DoodleLib/Metadata/Metadata_cpp.h>
// clang-format on
#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/core/ContainerDevice.h>
#include <google/protobuf/util/time_util.h>
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
  auto k_filter = std::make_shared<rpc_filter::filter>();
  k_filter->set_meta_type(Metadata::meta_type::project_root);
  auto k_list = FilterMetadata(k_filter);
  std::vector<ProjectPtr> k_out_list{};
  for (auto& i : k_list) {
    k_out_list.emplace_back(std::dynamic_pointer_cast<Project>(i));
  }
  return k_out_list;
}
std::vector<MetadataPtr> RpcMetadataClient::GetChild(const MetadataConstPtr& in_metadataPtr) {
  auto k_filter = std::make_shared<rpc_filter::filter>();
  k_filter->set_parent_id(in_metadataPtr->getId());
  return FilterMetadata(k_filter);
}
// void RpcMetadataClient::GetMetadata(const MetadataPtr& in_metadataPtr) {
//   auto k_filter = std::make_shared<rpc_filter::filter>();
//   k_filter->set_id(in_metadataPtr->getId());
//   FilterMetadata(k_filter).front();
  
// }
void RpcMetadataClient::InstallMetadata(const MetadataPtr& in_metadataPtr) {
  if (in_metadataPtr->isInstall())
    return;

  grpc::ClientContext k_context{};
  DataDb k_in_db{};

  k_in_db.set_uuidpath(in_metadataPtr->getUrlUUID().generic_string());
  k_in_db.mutable_m_type()->set_value(
      magic_enum::enum_cast<doodle::DataDb::meta_type>(in_metadataPtr->get_meta_type_int()).value());
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
std::vector<MetadataPtr> RpcMetadataClient::FilterMetadata(const rpc_filter::rpc_filter_ptr& in_filter_ptr) {
  std::vector<MetadataPtr> k_list{};
  grpc::ClientContext k_context{};
  DataDb_Filter k_filter{std::move(*in_filter_ptr)};
  auto k_r = p_stub->FilterMetadata(&k_context, k_filter);
  DataDb k_db;
  while (k_r->Read(&k_db)) {
    if (auto k_i = Metadata::from_DataDb(k_db); k_i)
      k_list.push_back(k_i);
  }
  auto status = k_r->Finish();

  if (!status.ok())
    throw DoodleError{status.error_message()};
  return k_list;
}
rpc_filter::filter::filter()
    : _id(),
      _parent_id(),
      _meta_type(),
      _begin(),
      _end() {
}
void rpc_filter::filter::set_id(std::int64_t in_id) {
  _id = in_id;
}
void rpc_filter::filter::set_parent_id(std::int64_t in_patent_id) {
  _parent_id = in_patent_id;
}
void rpc_filter::filter::set_meta_type(Metadata::meta_type in_meta_type) {
  _meta_type = in_meta_type;
}
void rpc_filter::filter::set_begin_time(const rpc_filter::filter::time_point& in_time) {
  _begin = in_time;
}
void rpc_filter::filter::set_end_time(const rpc_filter::filter::time_point& in_time) {
  _end = in_time;
}
void rpc_filter::filter::set_range(const rpc_filter::filter::time_point& in_begin, const rpc_filter::filter::time_point& in_end) {
  _begin = in_begin;
  _end   = in_end;
}
rpc_filter::filter::operator DataDb_Filter() const {
  DataDb_Filter k_tmp{};
  if (_id)
    k_tmp.set_id(*_id);
  if (_parent_id)
    k_tmp.mutable_parent()->set_value(*_parent_id);
  if (_meta_type)
    k_tmp.mutable_m_type()->set_value(
        magic_enum::enum_cast<DataDb::meta_type>(magic_enum::enum_integer(*_meta_type)).value());
  if (_begin && _end) {
    auto k_begin_timestamp = google::protobuf::util::TimeUtil::TimeTToTimestamp(
        std::chrono::system_clock::to_time_t(*_begin));
    auto k_end_timestamp = google::protobuf::util::TimeUtil::TimeTToTimestamp(
        std::chrono::system_clock::to_time_t(*_end));
    k_tmp.mutable_begin_time()->CopyFrom(k_begin_timestamp);
    k_tmp.mutable_end_time()->CopyFrom(k_end_timestamp);
  }
  return k_tmp;
}

}  // namespace doodle
