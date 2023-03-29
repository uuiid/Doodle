#include "redirection_path_info.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/generate/core/sql_sql.h>
#include <doodle_core/logger/logger.h>

#include <boost/filesystem/path.hpp>

#include "metadata/metadata.h"
#include "metadata/redirection_path_info.h"
#include <algorithm>
#include <entt/entity/fwd.hpp>
#include <lib_warp/enum_template_tool.h>
#include <magic_enum.hpp>
#include <memory>
#include <sqlpp11/aggregate_functions/count.h>
#include <sqlpp11/insert.h>
#include <sqlpp11/parameter.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/update.h>
#include <string>

namespace doodle::database_n {
namespace sql = doodle_database;
void sql_com<doodle::redirection_path_info>::insert(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  sql::RedirectionPathInfo l_table{};
  sql::RpiSearchPath l_path_table{};

  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.redirectionPath     = sqlpp::parameter(l_table.redirectionPath),
      l_table.redirectionFileName = sqlpp::parameter(l_table.redirectionFileName),
      l_table.entityId            = sqlpp::parameter(l_table.entityId)
  ));

  for (auto& l_h : l_handles) {
    auto& l_r_p_i                    = l_h.get<redirection_path_info>();

    // TODO:更新列表数据库
    l_pre.params.redirectionFileName = l_r_p_i.file_name_.string();
    l_pre.params.entityId            = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());

    auto l_r                         = l_conn(l_pre);
    DOODLE_LOG_INFO(
        "插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<redirection_path_info>().get_name()
    );
  }
}

void sql_com<doodle::redirection_path_info>::update(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  sql::RedirectionPathInfo l_table{};

  auto l_pre = l_conn.prepare(sqlpp::update(l_table)
                                  .set(
                                      l_table.redirectionPath     = sqlpp::parameter(l_table.redirectionPath),
                                      l_table.redirectionFileName = sqlpp::parameter(l_table.redirectionFileName)
                                  )
                                  .where(l_table.entityId == sqlpp::parameter(l_table.entityId)));
  for (auto& l_h : l_handles) {
    auto& l_r_p_i                    = l_h.get<redirection_path_info>();
    // TODO:更新列表数据库
    l_pre.params.redirectionFileName = l_r_p_i.file_name_.string();
    l_pre.params.entityId            = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());

    auto l_r                         = l_conn(l_pre);
    DOODLE_LOG_INFO(
        "插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<redirection_path_info>().get_name()
    );
  }
}
void sql_com<doodle::redirection_path_info>::select(
    conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle
) {
  auto& l_conn = *in_ptr;
  sql::RedirectionPathInfo l_table{};
  std::vector<redirection_path_info> l_r_p_i;
  std::vector<entt::entity> l_entts;
  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entityId)).from(l_table).where(l_table.entityId.is_not_null()))) {
    l_r_p_i.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  for (auto& row : l_conn(sqlpp::select(l_table.entityId, l_table.redirectionPath, l_table.redirectionFileName)
                              .from(l_table)
                              .where(l_table.entityId.is_null()))) {
    redirection_path_info l_r{};
    l_r.file_name_ = row.redirectionFileName.value();
    // TODO：更新列表
    auto l_id      = row.entityId.value();
    if (in_handle.find(l_id) != in_handle.end()) {
      l_r_p_i.emplace_back(std::move(l_r));
      l_entts.emplace_back(in_handle.at(l_id));
      DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
    } else {
      DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
    }
  }
  reg_->insert(l_entts.begin(), l_entts.end(), l_r_p_i.begin());
}
void sql_com<doodle::redirection_path_info>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<sql::RedirectionPathInfo>(in_ptr, in_handle);
}
}  // namespace doodle::database_n