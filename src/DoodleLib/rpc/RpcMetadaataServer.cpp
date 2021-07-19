//
// Created by TD on 2021/5/25.
//

#include "RpcMetadaataServer.h"

#include <DoodleLib/Metadata/Metadata_cpp.h>
#include <DoodleLib/core/CoreSql.h>
#include <Logger/Logger.h>
#include <core/MetadataTabSql.h>
#include <libWarp/protobuf_warp_cpp.h>
#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>

#include <cereal/archives/portable_binary.hpp>

namespace doodle {

std::string RpcMetadaataServer::get_cache_and_file(const FSys::path &key) {
  auto k_key = key.generic_string();

  if (p_cache.Cached(k_key)) {
    return p_cache.Get(k_key);
  } else {
    if (!FSys::exists(key.parent_path()))
      FSys::create_directories(key.parent_path());
    FSys::ifstream k_ifstream{k_key, std::ios::in | std::ios::binary};
    if (k_ifstream.is_open() && k_ifstream.good()) {
      auto str = std::string{
          std::istreambuf_iterator<char>(k_ifstream),
          std::istreambuf_iterator<char>()};
      p_cache.Put(k_key, str);
      return str;
    } else
      return {};
  }
}

void RpcMetadaataServer::put_cache_and_file(const FSys::path &key, const std::string &value) {
  if (!FSys::exists(key.parent_path()))
    FSys::create_directories(key.parent_path());
  // FSys::ofstream k_ofstream{key, std::ios::out | std::ios::binary};
  // k_ofstream.write(value.data(), value.size());
  p_cache.Put(key.generic_string(), value);
}

RpcMetadaataServer::RpcMetadaataServer()
    : p_set(CoreSet::getSet()),
      p_thread(),
      p_cache(1024 * 1024, caches::LRUCachePolicy<std::string>(), [this](const std::string &path, const std::string &value) {
        if (value.size() == 0)
          return;
        FSys::ofstream k_ofstream{path, std::ios::out | std::ios::binary};
        k_ofstream.write(value.data(), value.size());
      }) {
}

grpc::Status RpcMetadaataServer::GetProject(grpc::ServerContext *context, const google::protobuf::Empty *request, DataVector *response) {
  auto k_conn = CoreSql::Get().getConnection();
  Metadatatab k_tab{};

  for (const auto &row : (*k_conn)(sqlpp::select(sqlpp::all_of(k_tab))
                                       .from(k_tab)
                                       .where(k_tab.metaType == magic_enum::enum_integer(Metadata::meta_type::project_root)))) {
    auto k_data = response->add_data();
    k_data->set_id(row.id.value());
    k_data->set_uuidpath(std::string{row.uuidPath.value()});
    k_data->set_m_type(
        magic_enum::enum_cast<doodle::DataDb::meta_type>(row.metaType.value())
            .value_or(doodle::DataDb::meta_type::DataDb_meta_type_unknown_file));
    k_data->update_time();
    auto k_item = k_data->mutable_update_time();
    /// 这个到时候还要重新斟酌一下，有没有更快的转换方案
    auto k_time = std::chrono::system_clock::time_point{row.updateTime.value()};
    //    auto time   = google::protobuf::util::TimeUtil::TimeTToTimestamp(std::chrono::system_clock::to_time_t(k_time));
    *k_item = google::protobuf::util::TimeUtil::TimeTToTimestamp(std::chrono::system_clock::to_time_t(k_time));

    DOODLE_LOG_DEBUG(fmt::format("id: {} uuidPath: {}", row.id.value(), row.uuidPath.value()))

    auto k_path = getPath(row.uuidPath.value());
    auto k_str  = get_cache_and_file(k_path);
    if (k_str.empty())
      continue;
    k_data->mutable_metadata_cereal()->set_value(std::move(k_str));
  }

  return grpc::Status::OK;
}
grpc::Status RpcMetadaataServer::GetChild(grpc::ServerContext *context, const DataDb *request, DataVector *response) {
  auto k_conn = CoreSql::Get().getConnection();
  Metadatatab k_tab{};
  auto k_date = response->mutable_data();

  for (const auto &row : (*k_conn)(sqlpp::select(sqlpp::all_of(k_tab))
                                       .from(k_tab)
                                       .where(k_tab.parent == request->id()))) {
    DataDb k_db{};
    k_db.set_id(row.id.value());
    k_db.mutable_parent()->set_value(row.parent.value());
    k_db.set_uuidpath(std::string{row.uuidPath.value()});
    k_db.set_m_type(
        magic_enum::enum_cast<doodle::DataDb::meta_type>(row.metaType.value())
            .value_or(doodle::DataDb::meta_type::DataDb_meta_type_unknown_file));
    auto k_time      = std::chrono::system_clock::time_point{row.updateTime.value()};
    auto k_timestamp = google::protobuf::util::TimeUtil::TimeTToTimestamp(std::chrono::system_clock::to_time_t(k_time));
    k_db.mutable_update_time()->CopyFrom(k_timestamp);

    ///@warning 这里是要读取数据的，但是请记得添加缓存

    auto k_path = getPath(row.uuidPath.value());
    auto k_str  = get_cache_and_file(k_path);
    if (k_str.empty()) {
      DOODLE_LOG_WARN("id: {} uuidPath: {} 数据无效, 进行删除! ", row.id.value(), row.uuidPath.value())
      continue;
    }
    k_db.mutable_metadata_cereal()->set_value(std::move(k_str));

    DOODLE_LOG_DEBUG(fmt::format("id: {} uuidPath: {}", row.id.value(), row.uuidPath.value()))

    k_date->Add(std::move(k_db));
  }
  return grpc::Status::OK;
}

grpc::Status RpcMetadaataServer::GetMetadata(grpc::ServerContext *context, const DataDb *request, DataDb *response) {
  auto k_conn = CoreSql::Get().getConnection();
  Metadatatab k_tab{};
  for (const auto &row : (*k_conn)(sqlpp::select(sqlpp::all_of(k_tab))
                                       .from(k_tab)
                                       .where(k_tab.id == request->id()))) {
    ///设置一些普遍值
    response->set_id(row.id.value());
    response->mutable_parent()->set_value(row.parent.value());
    response->set_m_type(
        magic_enum::enum_cast<doodle::DataDb::meta_type>(row.metaType.value())
            .value_or(doodle::DataDb::meta_type::DataDb_meta_type_unknown_file));
    //    response->set_uuidpath(std::string{row.uuidPath.value()});
    //
    //    auto k_time = std::chrono::system_clock::time_point{row.updateTime.value()};
    //    auto k_timestamp = google::protobuf::util::TimeUtil::TimeTToTimestamp(std::chrono::system_clock::to_time_t(k_time));
    //    response->mutable_update_time()->CopyFrom(k_timestamp);

    auto k_path = getPath(row.uuidPath.value());
    auto k_str  = get_cache_and_file(k_path);
    if (k_str.empty())
      return grpc::Status::CANCELLED;

    DOODLE_LOG_DEBUG(fmt::format("id: {} uuidPath: {}", row.id.value(), row.uuidPath.value()))
    response->mutable_metadata_cereal()->set_value(std::move(k_str));
  }

  return grpc::Status::OK;
}
grpc::Status RpcMetadaataServer::InstallMetadata(grpc::ServerContext *context, const DataDb *request, DataDb *response) {
  auto k_conn = CoreSql::Get().getConnection();
  Metadatatab k_tab{};

  auto k_in = sqlpp::dynamic_insert_into(*k_conn, k_tab).dynamic_set();
  k_in.insert_list.add(k_tab.uuidPath = request->uuidpath());
  k_in.insert_list.add(k_tab.metaType = magic_enum::enum_integer(request->m_type()));
  if (request->has_parent()) {
    k_in.insert_list.add(k_tab.parent = request->parent().value());
  }
  auto k_id = (*k_conn)(k_in);

  if (k_id < 0) {
    DOODLE_LOG_DEBUG("插入数据库失败")
    return {grpc::StatusCode::RESOURCE_EXHAUSTED, "插入数据库失败"};
  }

  response->set_id(k_id);
  DOODLE_LOG_DEBUG(fmt::format("插入数据库 id: {}", k_id))

  auto path = getPath(request->uuidpath());
  put_cache_and_file(path, request->metadata_cereal().value());

  return grpc::Status::OK;
}
grpc::Status RpcMetadaataServer::DeleteMetadata(grpc::ServerContext *context, const DataDb *request, DataDb *response) {
  auto k_conn = CoreSql::Get().getConnection();
  Metadatatab k_tab{};

  auto k_path = getPath(request->uuidpath());
  // if (!p_cache.Remove(k_path.generic_string()))
  //   return {grpc::StatusCode::NOT_FOUND, "未找到缓存"};

  auto id = (*k_conn)(sqlpp::remove_from(k_tab).where(k_tab.id == request->id()));
  response->set_id(id);

  auto k_new_p = get_delete_path(request->uuidpath());
  if (FSys::exists(k_path)) {
    if (!FSys::exists(k_new_p.parent_path()))
      FSys::create_directories(k_new_p.parent_path());
    FSys::rename(k_path, k_new_p);

    DOODLE_LOG_DEBUG(fmt::format("delete id {}", id))
    DOODLE_LOG_WARN(fmt::format("移动文件 {} ---> {}", k_path, k_new_p))
  }
  return grpc::Status::OK;
}

grpc::Status RpcMetadaataServer::UpdateMetadata(grpc::ServerContext *context, const DataDb *request, DataDb *response) {
  auto k_conn = CoreSql::Get().getConnection();
  Metadatatab k_tab{};
  if (request->has_parent())
    (*k_conn)(
        sqlpp::update(k_tab).where(k_tab.id == request->id()).set(k_tab.parent = request->parent().value()));

  auto path = getPath(request->uuidpath());
  put_cache_and_file(path, request->metadata_cereal().value());

  DOODLE_LOG_DEBUG(fmt::format("id: {} update: {}", request->id(), path))
  return grpc::Status::OK;
}

}  // namespace doodle
