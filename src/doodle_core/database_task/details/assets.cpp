#include "assets.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/generate/core/sql_sql.h>
#include <doodle_core/logger/logger.h>

#include "metadata/assets.h"
#include "metadata/metadata.h"
#include <algorithm>
#include <entt/entity/fwd.hpp>
#include <lib_warp/enum_template_tool.h>
#include <magic_enum.hpp>
#include <sqlpp11/aggregate_functions/count.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
#include <string>
#include <vector>

namespace doodle::database_n {
namespace sql = doodle_database;
void sql_com<doodle::assets>::insert(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_id) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_id | ranges::views::transform([&](std::int64_t in_entity) {
                     return entt::handle{*reg_, num_to_enum<entt::entity>(in_entity)};
                   }) |
                   ranges::to_vector;
  sql::Assets l_table{};

  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.assetsPath = sqlpp::parameter(l_table.assetsPath), l_table.entityId = sqlpp::parameter(l_table.entityId)
  ));

  for (auto& l_h : l_handles) {
    auto& l_shot            = l_h.get<assets>();
    l_pre.params.assetsPath = l_shot.p_path.string();
    l_pre.params.entityId   = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    auto l_r                = l_conn(l_pre);
    DOODLE_LOG_INFO("插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<assets>().get_name());
  }
}

void sql_com<doodle::assets>::update(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_id) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_id | ranges::views::transform([&](std::int64_t in_entity) {
                     return entt::handle{*reg_, num_to_enum<entt::entity>(in_entity)};
                   }) |
                   ranges::to_vector;
  sql::Assets l_table{};

  auto l_pre = l_conn.prepare(sqlpp::update(l_table)
                                  .set(l_table.assetsPath = sqlpp::parameter(l_table.assetsPath))
                                  .where(l_table.entityId == sqlpp::parameter(l_table.entityId)));
  for (auto& l_h : l_handles) {
    auto& l_shot            = l_h.get<assets>();
    l_pre.params.assetsPath = l_shot.p_path.string();
    l_pre.params.entityId   = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());

    auto l_r                = l_conn(l_pre);
    DOODLE_LOG_INFO("更新数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<assets>().get_name());
  }
}

void sql_com<doodle::assets>::select(conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle) {
  auto& l_conn = *in_ptr;
  sql::Assets l_table{};
  std::vector<assets> l_assets;
  std::vector<entt::entity> l_entts;
  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entityId)).from(l_table).where(l_table.entityId.is_not_null()))) {
    l_assets.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }
  for (auto& row : l_conn(sqlpp::select(l_table.entityId, l_table.assetsPath)
                              .from(l_table)
                              .where(l_table.entityId.is_null()))) {
    assets l_s{};
    l_s.p_path      = row.assetsPath.value();
    auto l_id       = row.entityId.value();
    if (in_handle.find(l_id) != in_handle.end()) {
      l_assets.emplace_back(std::move(l_s));
      l_entts.emplace_back(in_handle.at(l_id));
      DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
    } else {
      DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
    }
  }
  reg_->insert(l_entts.begin(), l_entts.end(),l_assets.begin());
}
void sql_com<doodle::assets>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<sql::Assets>(in_ptr, in_handle);
}

}  // namespace doodle::database_n