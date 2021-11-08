//
// Created by TD on 2021/5/25.
//

#include "rpc_metadaata_server.h"

#include <doodle_lib/Logger/logger.h>
#include <doodle_lib/core/core_sql.h>
#include <doodle_lib/generate/core/metadatatab_sql.h>
#include <doodle_lib/generate/core/usertab_sql.h>
#include <doodle_lib/lib_warp/protobuf_warp_cpp.h>
#include <doodle_lib/metadata/metadata_cpp.h>
#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle {

std::string rpc_metadaata_server::get_cache_and_file(const FSys::path &key) {
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

void rpc_metadaata_server::put_cache_and_file(const FSys::path &key, const std::string &value) {
  if (!FSys::exists(key.parent_path()))
    FSys::create_directories(key.parent_path());
  FSys::ofstream k_ofstream{key, std::ios::out | std::ios::binary};
  k_ofstream.write(value.data(), value.size());
  p_cache.Put(key.generic_string(), value);
}

rpc_metadaata_server::rpc_metadaata_server()
    : p_set(core_set::getSet()),
      p_thread(),
      p_cache(
#ifdef NDEBUG
          1024 * 1024
#else
          10
#endif
          ,
          caches::LRUCachePolicy<std::string>(), [this](const std::string &path, const std::string &value) {
            if (value.empty())
              return;
            FSys::ofstream k_ofstream{path, std::ios::out | std::ios::binary};
            k_ofstream.write(value.data(), value.size());
          }) {
}

grpc::Status rpc_metadaata_server::install_metadata(grpc::ServerContext *context, const metadata_database *request, metadata_database *response) {
  auto k_conn = core_sql::Get().get_connection();
  Metadatatab k_tab{};

  auto k_in = sqlpp::dynamic_insert_into(*k_conn, k_tab).dynamic_set();
  k_in.insert_list.add(k_tab.uuidPath = request->uuid_path());
  k_in.insert_list.add(k_tab.metaType = request->m_type().value());
  if (request->has_parent()) {
    k_in.insert_list.add(k_tab.parent = request->parent().value());
  }
  if (request->has_update_time()) {
    auto k_time = std::chrono::system_clock::from_time_t(
        google::protobuf::util::TimeUtil::TimestampToTimeT(request->update_time()));
    k_in.insert_list.add(k_tab.updateTime = k_time);
  }

  if (request->has_episode()) {
    k_in.insert_list.add(k_tab.episode = request->episode().value());
  }

  if (request->has_shot()) {
    k_in.insert_list.add(k_tab.shot = request->shot().value());
  }

  if (request->has_season()) {
    k_in.insert_list.add(k_tab.season = request->season().value());
  }
  if (request->has_assets()) {
    k_in.insert_list.add(k_tab.assetsP = request->assets().value());
  }

  auto k_id = (*k_conn)(k_in);

  if (k_id < 0) {
    DOODLE_LOG_DEBUG("插入数据库失败")
    return {grpc::StatusCode::RESOURCE_EXHAUSTED, "插入数据库失败"};
  }

  response->set_id(k_id);
  DOODLE_LOG_DEBUG(fmt::format("插入数据库 id: {}", k_id))

  if (!request->metadata_cereal().value().empty()) {
    auto path = getPath(request->uuid_path());
    put_cache_and_file(path, request->metadata_cereal().value());
  }

  return grpc::Status::OK;
}
grpc::Status rpc_metadaata_server::delete_metadata(grpc::ServerContext *context, const metadata_database *request, metadata_database *response) {
  auto k_conn = core_sql::Get().get_connection();
  Metadatatab k_tab{};

  auto k_path = getPath(request->uuid_path());
  // if (!p_cache.Remove(k_path.generic_string()))
  //   return {grpc::StatusCode::NOT_FOUND, "未找到缓存"};

  auto id     = (*k_conn)(sqlpp::remove_from(k_tab).where(k_tab.id == request->id()));
  response->set_id(id);

  auto k_new_p = get_delete_path(request->uuid_path());
  if (FSys::exists(k_path)) {
    if (!FSys::exists(k_new_p.parent_path()))
      FSys::create_directories(k_new_p.parent_path());
    FSys::rename(k_path, k_new_p);

    DOODLE_LOG_DEBUG(fmt::format("delete id {}", id))
    DOODLE_LOG_WARN(fmt::format("移动文件 {} ---> {}", k_path, k_new_p))
  }
  return grpc::Status::OK;
}

grpc::Status rpc_metadaata_server::update_metadata(grpc::ServerContext *context, const metadata_database *request, metadata_database *response) {
  auto k_conn = core_sql::Get().get_connection();
  Metadatatab k_tab{};
  auto k_sql = sqlpp::dynamic_update(*k_conn, k_tab).where(k_tab.id == request->id()).dynamic_set();
  if (request->has_parent())
    k_sql.assignments.add(k_tab.parent = request->parent().value());
  if (request->has_update_time()) {
    auto k_time = std::chrono::system_clock::from_time_t(
        google::protobuf::util::TimeUtil::TimestampToTimeT(request->update_time()));

    k_sql.assignments.add(k_tab.updateTime = k_time);
  }
  if (request->has_m_type()) {
    k_sql.assignments.add(k_tab.metaType = request->m_type().value());
  }
  if (request->has_episode()) {
    k_sql.assignments.add(k_tab.episode = request->episode().value());
  }

  if (request->has_shot()) {
    k_sql.assignments.add(k_tab.shot = request->shot().value());
  }

  if (request->has_season()) {
    k_sql.assignments.add(k_tab.season = request->season().value());
  }
  if (request->has_assets()) {
    k_sql.assignments.add(k_tab.assetsP = request->assets().value());
  }

  if (request->has_parent() || request->has_update_time() || request->has_m_type())
    (*k_conn)(k_sql);

  if (!request->metadata_cereal().value().empty()) {
    auto path = getPath(request->uuid_path());
    put_cache_and_file(path, request->metadata_cereal().value());
    DOODLE_LOG_DEBUG(fmt::format("id: {} update: {}", request->id(), path))
  }
  DOODLE_LOG_DEBUG(fmt::format("id: {}", request->id()))

  return grpc::Status::OK;
}
grpc::Status rpc_metadaata_server::filter_metadata(grpc::ServerContext *context,
                                                   const metadata_database_filter *request, grpc::ServerWriter<metadata_database> *writer) {
  auto k_conn = core_sql::Get().get_connection();
  Metadatatab k_tab{};
  auto k_select = sqlpp::dynamic_select(*k_conn, sqlpp::all_of(k_tab)).from(k_tab).dynamic_where();

  if (request->has_begin_time() && request->has_end_time()) {
    auto k_time_begin = std::chrono::system_clock::from_time_t(
        google::protobuf::util::TimeUtil::TimestampToTimeT(request->begin_time()));
    auto k_time_end = std::chrono::system_clock::from_time_t(
        google::protobuf::util::TimeUtil::TimestampToTimeT(request->end_time()));
    k_select.where.add(k_tab.updateTime > k_time_begin && k_tab.updateTime < k_time_end);
  }
  if (request->has_m_type()) {
    k_select.where.add(k_tab.metaType == request->m_type().value());
  }
  if (request->id() != 0) {
    k_select.where.add(k_tab.id == request->id());
  }
  if (request->has_parent()) {
    k_select.where.add(k_tab.parent == request->parent().value());
  }

  if (request->has_episode()) {
    k_select.where.add(k_tab.episode == request->episode().value());
  }

  if (request->has_shot()) {
    k_select.where.add(k_tab.shot == request->shot().value());
  }

  if (request->has_season()) {
    k_select.where.add(k_tab.season == request->season().value());
  }
  if (request->has_assets()) {
    k_select.where.add(k_tab.assetsP.like(request->assets().value()));
  }
  if (request->has_beg_off_id()) {
    k_select.where.add(k_tab.id >= request->beg_off_id().value());
    if (request->has_off_size()) {
      k_select.limit(request->off_size().value());
    } else {
      k_select.limit(1000u);
    }
  }

  for (const auto &row : (*k_conn)(k_select)) {
    metadata_database k_db;
    k_db.set_id(row.id.value());
    k_db.mutable_parent()->set_value(row.parent.value());
    k_db.set_uuid_path(std::string{row.uuidPath.value()});
    k_db.mutable_m_type()->set_value(row.metaType.value());
    auto k_time      = std::chrono::system_clock::time_point{row.updateTime.value()};
    auto k_timestamp = google::protobuf::util::TimeUtil::TimeTToTimestamp(
        std::chrono::system_clock::to_time_t(k_time));
    k_db.mutable_update_time()->CopyFrom(k_timestamp);

    if (!row.season.is_null()) {
      k_db.mutable_season()->set_value(row.season.value());
    }
    if (!row.episode.is_null()) {
      k_db.mutable_episode()->set_value(row.episode.value());
    }
    if (!row.shot.is_null()) {
      k_db.mutable_shot()->set_value(row.shot.value());
    }
    if (!row.assetsP.is_null()) {
      k_db.mutable_assets()->set_value(row.assetsP.value());
    }
    ///@warning 这里是要读取数据的，但是请记得添加缓存
    auto k_path = getPath(row.uuidPath.value());
    auto k_str  = get_cache_and_file(k_path);
    if (k_str.empty()) {
      DOODLE_LOG_WARN("id: {} uuidPath: {} 数据无效, 进行删除! ", row.id.value(), row.uuidPath.value())
      continue;
    }
    k_db.mutable_metadata_cereal()->set_value(std::move(k_str));
    if (!writer->Write(k_db))
      return grpc::Status::CANCELLED;

    DOODLE_LOG_DEBUG(fmt::format("id: {} uuidPath: {}", row.id.value(), row.uuidPath.value()))
  }

  return grpc::Status::OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////                                             /////////////////////////////////////
/////////////////////////////////////              user表                          /////////////////////////////////////
/////////////////////////////////////                                             /////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

grpc::Status rpc_metadaata_server::install_user_date(::grpc::ServerContext *context, const ::doodle::user_database *request, ::doodle::user_database *response) {
  auto k_conn = core_sql::Get().get_connection();
  Usertab k_tab{};
  auto k_sql = sqlpp::insert_into(k_tab).set(
      k_tab.uuidPath        = request->uuidpath(),
      k_tab.userName        = request->user_name(),
      k_tab.permissionGroup = request->permission_group());
  auto k_id = (*k_conn)(k_sql);
  if (k_id < 0) {
    DOODLE_LOG_DEBUG("插入数据库失败")
    return {grpc::StatusCode::RESOURCE_EXHAUSTED, "插入数据库失败"};
  }

  response->set_id(k_id);
  DOODLE_LOG_DEBUG(fmt::format("插入数据库 id: {}", k_id))
  if (!request->userdata_cereal().value().empty()) {
    auto k_path = getPath(request->uuidpath());
    put_cache_and_file(k_path, request->userdata_cereal().value());
  }
  return grpc::Status::OK;
}
grpc::Status rpc_metadaata_server::update_user_date(::grpc::ServerContext *context, const ::doodle::user_database *request, ::doodle::user_database *response) {
  if (!request->userdata_cereal().value().empty()) {
    auto k_path = getPath(request->uuidpath());
    put_cache_and_file(k_path, request->userdata_cereal().value());
  }
  return grpc::Status::OK;
}
grpc::Status rpc_metadaata_server::delete_user_date(::grpc::ServerContext *context, const ::doodle::user_database_filter *request, ::doodle::user_database *response) {
  auto k_conn = core_sql::Get().get_connection();
  Usertab k_tab{};

  auto k_path = getPath(request->uuidpath());

  auto id     = (*k_conn)(sqlpp::remove_from(k_tab).where(k_tab.id == request->id()));
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
grpc::Status rpc_metadaata_server::filter_user_date(::grpc::ServerContext *context, const ::doodle::user_database_filter *request, ::grpc::ServerWriter< ::doodle::user_database> *writer) {
  auto k_conn = core_sql::Get().get_connection();
  Usertab k_tab{};

  auto k_select = sqlpp::dynamic_select(*k_conn, sqlpp::all_of(k_tab)).from(k_tab).dynamic_where();
  if (!request->user_name().empty()) {
    k_select.where.add(k_tab.userName == request->user_name());
  }
  if (request->permission_group() != 0) {
    k_select.where.add(k_tab.permissionGroup == request->permission_group());
  }
  for (const auto &row : (*k_conn)(k_select)) {
    user_database k_db{};
    k_db.set_id(row.id);
    k_db.set_uuidpath(row.uuidPath.value());
    k_db.set_permission_group(row.permissionGroup.value());
    k_db.set_user_name(row.userName.value());

    auto k_path = getPath(row.uuidPath.value());
    auto k_str  = get_cache_and_file(k_path);
    if (k_str.empty()) {
      DOODLE_LOG_WARN("id: {} uuidPath: {} 数据无效, 进行删除! ", row.id.value(), row.uuidPath.value())
      continue;
    }
    k_db.mutable_userdata_cereal()->set_value(std::move(k_str));
    if (!writer->Write(k_db))
      return grpc::Status::CANCELLED;

    DOODLE_LOG_DEBUG(fmt::format("id: {} uuidPath: {}", row.id.value(), row.uuidPath.value()))
  }
  return grpc::Status::OK;
}

}  // namespace doodle
