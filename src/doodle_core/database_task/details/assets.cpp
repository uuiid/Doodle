#include "assets.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/logger/logger.h>

#include "metadata/assets.h"
#include "metadata/metadata.h"
#include "sqlpp11/parameter.h"
#include "sqlpp11/sqlite3/insert_or.h"
#include "sqlpp11/update.h"
#include <algorithm>
#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <lib_warp/enum_template_tool.h>
#include <magic_enum.hpp>
#include <sqlpp11/aggregate_functions/count.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
#include <string>
#include <vector>

namespace doodle::database_n {

void sql_com<doodle::assets>::insert(conn_ptr& in_ptr, const std::vector<entt::handle>& in_id) {
  auto& l_conn = *in_ptr;

  tables::assets l_table{};
  //  sqlpp::insert_into(l_table).c
  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.assets_path = sqlpp::parameter(l_table.assets_path),
      l_table.entity_id = sqlpp::parameter(l_table.entity_id), l_table.parent_id = sqlpp::parameter(l_table.parent_id)
  ));

  for (auto& l_h : in_id) {
    auto& l_shot             = l_h.get<assets>();
    l_pre.params.entity_id   = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    l_pre.params.assets_path = l_shot.p_path;
    if (auto l_p = l_h.get<assets>().get_parent(); l_p && l_p.any_of<database>()) {
      l_pre.params.parent_id = boost::numeric_cast<std::int64_t>(l_p.get<database>().get_id());
    }
    auto l_r = l_conn(l_pre);
    DOODLE_LOG_INFO("插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), entt::type_id<assets>().name());
  }
}

void sql_com<doodle::assets>::update(conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id) {
  auto& l_conn = *in_ptr;

  tables::assets l_table{};

  auto l_pre = l_conn.prepare(
      sqlpp::update(l_table)
          .set(
              l_table.assets_path = sqlpp::parameter(l_table.assets_path),
              l_table.parent_id   = sqlpp::parameter(l_table.parent_id)
          )
          .where(l_table.entity_id == sqlpp::parameter(l_table.entity_id) && l_table.id == sqlpp::parameter(l_table.id))

  );
  for (const auto& [id, l_h] : in_id) {
    auto& l_shot             = l_h.get<assets>();
    l_pre.params.id          = id;
    l_pre.params.entity_id   = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());

    l_pre.params.assets_path = l_shot.p_path;
    if (auto l_p = l_h.get<assets>().get_parent(); l_p && l_p.any_of<database>())
      l_pre.params.parent_id = boost::numeric_cast<std::int64_t>(l_p.get<database>().get_id());
    auto l_r = l_conn(l_pre);
    DOODLE_LOG_INFO("更新数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), entt::type_id<assets>().name());
  }
}

void sql_com<doodle::assets>::select(
    conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle, const registry_ptr& in_reg
) {
  auto& l_conn = *in_ptr;
  tables::assets l_table{};
  std::vector<assets> l_assets;
  std::vector<entt::entity> l_entts;
  std::map<entt::handle, entt::handle> l_ass_parent{};

  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
    l_assets.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }
  for (auto& row : l_conn(sqlpp::select(l_table.entity_id, l_table.assets_path, l_table.parent_id)
                              .from(l_table)
                              .where(l_table.entity_id.is_not_null()))) {
    assets l_s{};
    l_s.p_path = row.assets_path.value();
    auto l_id  = row.entity_id.value();

    if (in_handle.find(l_id) != in_handle.end()) {
      l_assets.emplace_back(std::move(l_s));
      l_entts.emplace_back(in_handle.at(l_id));
      if (auto l_p_id = row.parent_id.value(); !row.parent_id.is_null() && in_handle.find(l_p_id) != in_handle.end()) {
        l_ass_parent.emplace(in_handle.at(l_id), in_handle.at(l_p_id));
      }
      DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
    } else {
      DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
    }
  }
  in_reg->insert<doodle::assets>(l_entts.begin(), l_entts.end(), l_assets.begin());
  /// 向后插入, 防止找不到自身的句柄
  for (auto [l_c, l_p] : l_ass_parent) {
    l_p.get<assets>().add_child(l_c);
  }
}
void sql_com<doodle::assets>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<tables::assets>(in_ptr, in_handle);
}

}  // namespace doodle::database_n