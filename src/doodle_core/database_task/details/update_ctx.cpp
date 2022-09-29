//
// Created by TD on 2022/6/1.
//

#include "update_ctx.h"
#include <doodle_core/thread_pool/process_message.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/thread_pool/thread_pool.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/core/core_sql.h>
#include <doodle_core/core/file_sys.h>

#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/metadata/importance.h>
#include <doodle_core/metadata/organization.h>
#include <doodle_core/metadata/redirection_path_info.h>

#include <doodle_core/generate/core/sql_sql.h>
#include <doodle_core/database_task/sql_file.h>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>

#include <range/v3/all.hpp>

namespace doodle::database_n::details {

namespace sql = doodle_database;

namespace {
template <typename Type_T>
void _get_ctx_sql_data_(
    const entt::registry& in_registry,
    std::map<std::uint32_t, std::string>& in_data
) {
  if (in_registry.ctx().contains<Type_T>()) {
    auto l_json = nlohmann::json{};
    l_json      = in_registry.ctx().at<Type_T>();
    in_data.emplace(std::make_pair(entt::type_id<Type_T>().hash(), l_json.dump()));
  }
}
template <typename... Type_T>
void get_ctx_sql_data(
    const entt::registry& in_registry,
    std::map<std::uint32_t, std::string>& in_data
) {
  (_get_ctx_sql_data_<Type_T>(in_registry, in_data), ...);
}

void _select_ctx_(
    entt::registry& in_reg,
    sqlpp::sqlite3::connection& in_conn,
    const std::map<std::uint32_t, std::function<void(entt::registry& in_reg, const std::string& in_str)>>& in_fun_list
) {
  sql::Context l_context{};

  for (auto&& row : in_conn(
           sqlpp::select(l_context.comHash, l_context.jsonData)
               .from(l_context)
               .unconditionally()
       )) {
    if (auto l_f = in_fun_list.find(row.comHash.value());
        l_f != in_fun_list.end()) {
      in_fun_list.at(row.comHash.value())(in_reg, row.jsonData.value());
    }
  }
}
template <typename... Type>
void select_ctx_template(entt::registry& in_reg, sqlpp::sqlite3::connection& in_conn) {
  std::map<std::uint32_t, std::function<void(entt::registry & in_reg, const std::string& in_str)>>
      l_fun{
          std::make_pair(entt::type_id<Type>().hash(), [&](entt::registry& in_reg, const std::string& in_str) {
            auto l_json = nlohmann::json::parse(in_str);
            if (in_reg.ctx().template contains<Type>())
              in_reg.ctx().template erase<Type>();

            in_reg.ctx().template emplace<Type>(
                std::move(l_json.get<Type>())
            );
          })...};

  _select_ctx_(in_reg, in_conn, l_fun);
}

}  // namespace

void update_ctx::ctx(const entt::registry& in_registry, sqlpp::sqlite3::connection& in_connection) {
  std::map<std::uint32_t, std::string> data{};
#include "macro.h"
  get_ctx_sql_data<DOODLE_SQLITE_TYPE_CTX>(in_registry, data);

  sql::Context l_table{};

  auto l_par = in_connection.prepare(
      sqlpp::sqlite3::insert_or_replace_into(l_table)
          .set(l_table.comHash = sqlpp::parameter(l_table.comHash), l_table.jsonData = sqlpp::parameter(l_table.jsonData))
  );
  for (auto&& i : data) {
    l_par.params.comHash  = i.first;
    l_par.params.jsonData = i.second;
    in_connection(l_par);
  }
}

void update_ctx::select_ctx(entt::registry& in_registry, sqlpp::sqlite3::connection& in_connection) {
  select_ctx_template<DOODLE_SQLITE_TYPE_CTX>(in_registry, in_connection);
}

std::tuple<std::uint32_t, std::uint32_t> get_version(
    sqlpp::sqlite3::connection& in_conn
) {
  sql::DoodleInfo l_info{};

  for (auto&& row : in_conn(
           sqlpp::select(all_of(l_info)).from(l_info).unconditionally()
       )) {
    return std::make_tuple(boost::numeric_cast<std::uint32_t>(row.versionMajor.value()), boost::numeric_cast<std::uint32_t>(row.versionMinor.value()));
  }

  throw_exception(doodle_error{"无法检查到数据库版本 {}", g_reg()->ctx().at<database_info>().path_});
  return {};
}

void db_compatible::add_ctx_table(sqlpp::sqlite3::connection& in_conn) {
  in_conn.execute(std::string{::doodle::database_n::create_ctx_table});
  in_conn.execute(std::string{::doodle::database_n::create_ctx_table_index});
  in_conn.execute(std::string{::doodle::database_n::create_ctx_table_unique});
}

void db_compatible::add_entity_table(sqlpp::sqlite3::connection& in_conn) {
  in_conn.execute(std::string{::doodle::database_n::create_entity_table});
  in_conn.execute(std::string{::doodle::database_n::create_entity_table_index});
}

void db_compatible::add_component_table(sqlpp::sqlite3::connection& in_conn) {
  in_conn.execute(std::string{::doodle::database_n::create_com_table});
  in_conn.execute(std::string{::doodle::database_n::create_com_table_index_id});
  in_conn.execute(std::string{::doodle::database_n::create_com_table_index_hash});
  in_conn.execute(std::string{::doodle::database_n::create_com_table_trigger});
}

void db_compatible::add_version_table(sqlpp::sqlite3::connection& in_conn) {
  if (has_version_table(in_conn)) {
    return;
  }
  in_conn.execute(std::string{::doodle::database_n::create_version_table});
  sql::DoodleInfo l_info{};
  in_conn(sqlpp::sqlite3::insert_or_replace_into(l_info).set(
      l_info.versionMajor = version::build_info::get().version_major,
      l_info.versionMinor = version::build_info::get().version_minor
  ));
}

void db_compatible::set_version(sqlpp::sqlite3::connection& in_conn) {
  sql::DoodleInfo l_info{};
  in_conn(sqlpp::update(l_info).unconditionally().set(
      l_info.versionMajor = version::build_info::get().version_major,
      l_info.versionMinor = version::build_info::get().version_minor
  ));
}
bool db_compatible::has_version_table(sqlpp::sqlite3::connection& in_conn) {
  sql::SqliteMaster l_master{};
  auto l_item = in_conn(
      sqlpp::select(sqlpp::all_of(l_master))
          .from(l_master)
          .where(l_master.type == "table" && l_master.name == "doodle_info")
  );
  for (auto&& row : l_item) {
    return true;
  }
  return false;
}

bool db_compatible::has_metadatatab_table(sqlpp::sqlite3::connection& in_conn) {
  sql::SqliteMaster l_master{};
  auto l_item = in_conn(
      sqlpp::select(sqlpp::all_of(l_master))
          .from(l_master)
          .where(l_master.type == "table" && l_master.name == "metadatatab")
  );
  for (auto&& row : l_item) {
    return true;
  }
  return false;
}
void db_compatible::delete_metadatatab_table(sqlpp::sqlite3::connection& in_conn) {
  in_conn.execute(R"(drop table if exists metadatatab;)");
}
}  // namespace doodle::database_n::details
