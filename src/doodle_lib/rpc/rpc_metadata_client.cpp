//
// Created by TD on 2021/5/25.
//

#include "rpc_metadata_client.h"

#include <doodle_lib/Exception/exception.h>
// clang-format off
#include <doodle_lib/metadata/metadata_cpp.h>
// clang-format on
#include <doodle_lib/Logger/logger.h>
#include <doodle_lib/core/ContainerDevice.h>
//#include <consoleapi.h>
//#include <consoleapi2.h>
#include <doodle_lib/core/doodle_lib.h>
#include <google/protobuf/util/time_util.h>
#include <grpcpp/grpcpp.h>

namespace doodle {

rpc_metadata_client::rpc_metadata_client(const std::shared_ptr<grpc::Channel>& in_channel)
    : p_stub(metadata_server::NewStub(in_channel))
// p_channel(in_channel)
{
  //  auto k_s = p_channel->GetState(true);
}
std::vector<project_ptr> rpc_metadata_client::get_project() {
  auto k_filter = new_object<rpc_filter::filter>();
  k_filter->set_meta_type(metadata::meta_type::project_root);
  auto k_list = filter_metadata(k_filter);
  std::vector<project_ptr> k_out_list{};
  std::transform(
      k_list.begin(), k_list.end(), std::back_inserter(k_out_list),
      [](const metadata_ptr& in) {
        return std::dynamic_pointer_cast<project>(in);
      });
  return k_out_list;
}
std::vector<metadata_ptr> rpc_metadata_client::get_child(const metadata_const_ptr& in_metadataPtr) {
  auto k_filter = new_object<rpc_filter::filter>();
  k_filter->set_parent_id(in_metadataPtr->getId());
  return filter_metadata(k_filter);
}
// void RpcMetadataClient::GetMetadata(const MetadataPtr& in_metadataPtr) {
//   auto k_filter = new_object<rpc_filter::filter>();
//   k_filter->set_id(in_metadataPtr->getId());
//   FilterMetadata(k_filter).front();

// }
void rpc_metadata_client::install_metadata(const metadata_ptr& in_metadataPtr) {
  if (in_metadataPtr->is_install())
    return;

  grpc::ClientContext k_context{};
  metadata_database k_in_db{*in_metadataPtr};
  metadata_database k_out_db{};
  auto k_status = p_stub->install_metadata(&k_context, k_in_db, &k_out_db);
  if (k_status.ok()) {
    in_metadataPtr->p_id = k_out_db.id();
  } else {
    throw doodle_error{k_status.error_message()};
  }
  update_metadata(in_metadataPtr, false);
}
void rpc_metadata_client::delete_metadata(const metadata_const_ptr& in_metadataPtr) {
  if (!in_metadataPtr->is_install())
    return;

  grpc::ClientContext k_context{};
  metadata_database k_in_db{};
  k_in_db.set_id(in_metadataPtr->getId());
  k_in_db.set_uuid_path(in_metadataPtr->get_url_uuid().generic_string());

  metadata_database k_out_db{};
  auto k_status = p_stub->delete_metadata(&k_context, k_in_db, &k_out_db);
  DOODLE_LOG_WARN("删除数据 : {} 路径 {}", in_metadataPtr->getId(), in_metadataPtr->get_url_uuid())
  if (!k_status.ok()) {
    throw doodle_error{k_status.error_message()};
  }
}

void rpc_metadata_client::update_metadata(const metadata_const_ptr& in_metadataPtr, bool b_update_parent_id) {
  if (!in_metadataPtr->is_install())
    return;

  grpc::ClientContext k_context{};
  metadata_database k_in_db{*in_metadataPtr};
  metadata_database k_out_db{};
  auto k_status = p_stub->update_metadata(&k_context, k_in_db, &k_out_db);
  if (!k_status.ok()) {
    throw doodle_error{k_status.error_message()};
  }
}
std::vector<metadata_ptr> rpc_metadata_client::filter_metadata(const rpc_filter::rpc_filter_ptr& in_filter_ptr) {
  std::vector<metadata_ptr> k_list{};
  grpc::ClientContext k_context{};
  metadata_database_filter k_filter{std::move(*in_filter_ptr)};
  auto k_r = p_stub->filter_metadata(&k_context, k_filter);
  metadata_database k_db;
  while (k_r->Read(&k_db)) {
    if (auto k_i = metadata::from_DataDb(k_db); k_i)
      k_list.push_back(k_i);
  }
  auto status = k_r->Finish();

  if (!status.ok())
    throw doodle_error{status.error_message()};
  return k_list;
}

void rpc_metadata_client::install_metadata(const database& in_database) {
  grpc::ClientContext k_context{};
  metadata_database k_in_db{in_database};
  metadata_database k_out_db{};
  auto k_status = p_stub->install_metadata(&k_context, k_in_db, &k_out_db);
  if (k_status.ok()) {
    in_database.p_id = k_out_db.id();
  } else {
    throw doodle_error{k_status.error_message()};
  }
}

void rpc_metadata_client::delete_metadata(const database& in_database) {
  if (!in_database.is_install())
    return;

  grpc::ClientContext k_context{};
  metadata_database k_in_db{in_database};
  metadata_database k_out_db{};
  auto k_status = p_stub->delete_metadata(&k_context, k_in_db, &k_out_db);
  DOODLE_LOG_WARN("删除数据 : {} 路径 {}", in_database.get_id(), in_database.get_url_uuid())
  if (!k_status.ok()) {
    throw doodle_error{k_status.error_message()};
  }
}

void rpc_metadata_client::update_metadata(const database& in_database) {
  if (!in_database.is_install())
    return;

  grpc::ClientContext k_context{};
  metadata_database k_in_db{in_database};
  metadata_database k_out_db{};
  auto k_status = p_stub->update_metadata(&k_context, k_in_db, &k_out_db);
  if (!k_status.ok()) {
    throw doodle_error{k_status.error_message()};
  }
}

std::vector<metadata_database> rpc_metadata_client::select_metadata(const rpc_filter::rpc_filter_ptr& in_filter_ptr) {
  std::vector<metadata_database> k_list{};
  grpc::ClientContext k_context{};
  metadata_database_filter k_filter{std::move(*in_filter_ptr)};
  auto k_r = p_stub->filter_metadata(&k_context, k_filter);
  metadata_database k_db;

  auto k_reg = g_reg();
  while (k_r->Read(&k_db)) {
    k_list.push_back(std::move(k_db));
  }
  auto status = k_r->Finish();

  if (!status.ok())
    throw doodle_error{status.error_message()};
  return k_list;
}

std::vector<entt::entity> rpc_metadata_client::select_entity(const rpc_filter::rpc_filter_ptr& in_filter_ptr) {
  std::vector<entt::entity> k_list{};
  auto k_reg = g_reg();
  for (auto& k_db : select_metadata(in_filter_ptr)) {
    auto k_e  = k_reg->create();
    auto& k_d = k_reg->emplace<database>(k_e);
    k_d       = k_db;
    k_list.push_back(k_e);
  }
  return k_list;
}
rpc_filter::filter::filter()
    : _id(),
      _parent_id(),
      _meta_type(),
      _begin(),
      _end() {
}
void rpc_filter::filter::set_id(uint64_t in_id) {
  _id = in_id;
}
void rpc_filter::filter::set_parent_id(std::int64_t in_patent_id) {
  _parent_id = in_patent_id;
}
void rpc_filter::filter::set_meta_type(metadata::meta_type in_meta_type) {
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
rpc_filter::filter::operator metadata_database_filter() const {
  metadata_database_filter k_tmp{};
  if (_id)
    k_tmp.set_id(*_id);
  if (_parent_id)
    k_tmp.mutable_parent()->set_value(*_parent_id);
  if (_meta_type)
    k_tmp.mutable_m_type()->set_value(
        magic_enum::enum_cast<metadata_database::meta_type>(magic_enum::enum_integer(*_meta_type)).value());
  if (_begin && _end) {
    auto k_begin_timestamp = google::protobuf::util::TimeUtil::TimeTToTimestamp(
        chrono::to_time_t(*_begin));
    auto k_end_timestamp = google::protobuf::util::TimeUtil::TimeTToTimestamp(
        chrono::to_time_t(*_end));
    k_tmp.mutable_begin_time()->CopyFrom(k_begin_timestamp);
    k_tmp.mutable_end_time()->CopyFrom(k_end_timestamp);
  }
  return k_tmp;
}
void rpc_filter::filter::reset() {
  _id.reset();
  _parent_id.reset();
  _meta_type.reset();
  _begin.reset();
  _end.reset();
}
void rpc_filter::filter::set_range(const std::pair<time_point, time_point>& in_time) {
  _begin = in_time.first;
  _end   = in_time.second;
}

}  // namespace doodle
