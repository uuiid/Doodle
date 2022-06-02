//
// Created by TD on 2022/6/1.
//

#include "update_ctx.h"
#include <doodle_core/thread_pool/process_message.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/thread_pool/thread_pool.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/core/core_sql.h>

#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/metadata/importance.h>
#include <doodle_core/metadata/organization.h>
#include <doodle_core/metadata/redirection_path_info.h>

#include <doodle_core/generate/core/sql_sql.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>

#include <range/v3/all.hpp>

namespace doodle::database_n::details {

namespace sql = doodle_database;

namespace {
template <typename Type_T>
void _get_ctx_sql_data_(
    const entt::registry& in_registry,
    std::map<std::uint32_t, std::string>& in_data) {
  if (in_registry.ctx().contains<Type_T>()) {
    auto l_json = nlohmann::json{};
    l_json      = in_registry.ctx().at<Type_T>();
    in_data.emplace(std::make_pair(entt::type_id<Type_T>().hash(), l_json.dump()));
  }
}
template <typename... Type_T>
void get_ctx_sql_data(
    const entt::registry& in_registry,
    std::map<std::uint32_t, std::string>& in_data) {
  (_get_ctx_sql_data_<Type_T>(in_registry, in_data), ...);
}
}  // namespace

void update_ctx::ctx(const entt::registry& in_registry,
                     sqlpp::sqlite3::connection& in_connection) {
  std::map<std::uint32_t, std::string> data{};
#include "macro.h"
  get_ctx_sql_data<DOODLE_SQLITE_TYPE_CTX>(in_registry, data);

  sql::Context l_table{};

  auto l_par = in_connection.prepare(
      sqlpp::sqlite3::insert_or_replace_into(l_table)
          .set(l_table.comHash  = sqlpp::parameter(l_table.comHash),
               l_table.jsonData = sqlpp::parameter(l_table.jsonData)));
  for (auto&& i : data) {
    l_par.params.comHash  = i.first;
    l_par.params.jsonData = i.second;
    in_connection(l_par);
  }
}
}  // namespace doodle::database_n::details
