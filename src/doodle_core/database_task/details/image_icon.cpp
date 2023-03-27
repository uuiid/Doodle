#include "image_icon.h"
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/generate/core/sql_sql.h>
#include <doodle_core/logger/logger.h>

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
namespace sql = doodle_database;
void sql_com<doodle::image_icon>::insert(conn_ptr& in_ptr, const entt::observer& in_observer) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_observer | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  sql::ImageIcon l_table{};

  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.path = sqlpp::parameter(l_table.path), 
      l_table.entityId = sqlpp::parameter(l_table.entityId)
  ));

  for (auto& l_h : l_handles) {
    auto& l_shot          = l_h.get<image_icon>();
    l_pre.params.path  = l_shot.path.string();
    l_pre.params.entityId = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    auto l_r              = l_conn(l_pre);
    DOODLE_LOG_INFO("插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<image_icon>().get_name());
  }
}

void sql_com<doodle::image_icon>::update(conn_ptr& in_ptr, const entt::observer& in_observer) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_observer | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  sql::ImageIcon l_table{};

  auto l_pre = l_conn.prepare(
      sqlpp::update(l_table)
          .set(l_table.path = sqlpp::parameter(l_table.path))
          .where(l_table.entityId == sqlpp::parameter(l_table.entityId))
  );
  for (auto& l_h : l_handles) {
    auto& l_img          = l_h.get<image_icon>();
    l_pre.params.path   = l_img.path.string();
    l_pre.params.entityId = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());

    auto l_r              = l_conn(l_pre);
    DOODLE_LOG_INFO("更新数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<image_icon>().get_name());
  }
}
void sql_com<doodle::image_icon>::select(conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle) {
  auto& l_conn = *in_ptr;
  sql::ImageIcon l_table{};
  std::vector<image_icon> l_img;
  std::vector<entt::entity> l_entts;
  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entityId)).from(l_table).where(l_table.entityId.is_not_null()))) {
    l_img.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  for (auto& row : l_conn(sqlpp::select(l_table.entityId, l_table.path)
                              .from(l_table)
                              .where(l_table.entityId.is_null()))) {
    image_icon l_i{};
    l_i.path      = row.path.value();
    auto l_id       = row.entityId.value();
    if (in_handle.find(l_id) != in_handle.end()) {
      l_img.emplace_back(std::move(l_i));
      l_entts.emplace_back(in_handle.at(l_id));
      DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
    } else {
      DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
    }
  }
  reg_->insert(l_entts.begin(), l_entts.end(), l_img.begin());
}
void sql_com<doodle::image_icon>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<sql::ImageIcon>(in_ptr, in_handle);
}

}  // namespace doodle::database_n