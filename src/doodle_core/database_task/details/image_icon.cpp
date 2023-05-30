#include "image_icon.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/logger/logger.h>

#include "core/core_help_impl.h"
#include "metadata/image_icon.h"
#include "metadata/metadata.h"
#include <algorithm>
#include <entt/entity/fwd.hpp>
#include <lib_warp/enum_template_tool.h>
#include <magic_enum.hpp>
#include <sqlpp11/aggregate_functions/count.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::database_n {
void sql_com<doodle::image_icon>::insert(conn_ptr& in_ptr, const std::vector<entt::handle>& in_id) {
  auto& l_conn = *in_ptr;

  tables::image_icon l_table{};

  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.path = sqlpp::parameter(l_table.path), l_table.entity_id = sqlpp::parameter(l_table.entity_id)
  ));

  for (auto& l_h : in_id) {
    auto& l_shot           = l_h.get<image_icon>();
    l_pre.params.path      = l_shot.path.string();
    l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    auto l_r               = l_conn(l_pre);
    DOODLE_LOG_INFO("插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), entt::type_id<image_icon>().name());
  }
}

void sql_com<doodle::image_icon>::update(conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id) {
  auto& l_conn = *in_ptr;

  tables::image_icon l_table{};

  auto l_pre = l_conn.prepare(
      sqlpp::update(l_table)
          .set(l_table.path = sqlpp::parameter(l_table.path))
          .where(l_table.entity_id == sqlpp::parameter(l_table.entity_id) && l_table.id == sqlpp::parameter(l_table.id))
  );
  for (auto& [id, l_h] : in_id) {
    auto& l_img            = l_h.get<image_icon>();
    l_pre.params.path      = l_img.path.string();
    l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    l_pre.params.id        = id;

    auto l_r               = l_conn(l_pre);
    DOODLE_LOG_INFO("更新数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), entt::type_id<image_icon>().name());
  }
}
void sql_com<doodle::image_icon>::select(
    conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle, const registry_ptr& in_reg
) {
  auto& l_conn = *in_ptr;
  tables::image_icon l_table{};
  std::vector<image_icon> l_img;
  std::vector<entt::entity> l_entts;
  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
    l_img.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  for (auto& row :
       l_conn(sqlpp::select(l_table.entity_id, l_table.path).from(l_table).where(l_table.entity_id.is_not_null()))) {
    image_icon l_i{};
    l_i.path  = row.path.value();
    auto l_id = row.entity_id.value();
    if (in_handle.find(l_id) != in_handle.end()) {
      l_img.emplace_back(std::move(l_i));
      l_entts.emplace_back(in_handle.at(l_id));
      DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
    } else {
      DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
    }
  }
  in_reg->insert<doodle::image_icon>(l_entts.begin(), l_entts.end(), l_img.begin());
}
void sql_com<doodle::image_icon>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<tables::image_icon>(in_ptr, in_handle);
}

}  // namespace doodle::database_n