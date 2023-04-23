#include "assets_file.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/user.h>

#include "core/core_help_impl.h"
#include "metadata/assets_file.h"
#include "metadata/metadata.h"
#include <algorithm>
#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <lib_warp/enum_template_tool.h>
#include <magic_enum.hpp>
#include <sqlpp11/aggregate_functions/count.h>
#include <sqlpp11/parameter.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
#include <string>
#include <vector>

namespace doodle::database_n {
void sql_com<doodle::assets_file>::insert(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  auto& l_conn   = *in_ptr;
  auto l_handles = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  tables::assets_file l_table{};
  auto l_pre = l_conn.prepare(sqlpp::sqlite3::insert_or_replace_into(l_table).set(
      l_table.name = sqlpp::parameter(l_table.name), l_table.path = sqlpp::parameter(l_table.path),
      l_table.version = sqlpp::parameter(l_table.version), l_table.user_ref = sqlpp::parameter(l_table.user_ref),
      l_table.entity_id = sqlpp::parameter(l_table.entity_id)
  ));

  for (auto& l_h : l_handles) {
    auto& l_assets       = l_h.get<assets_file>();
    l_pre.params.name    = l_assets.name_attr();
    l_pre.params.path    = l_assets.path_attr().string();
    l_pre.params.version = l_assets.version_attr();
    if (auto l_h = l_assets.user_attr(); l_h && l_h.any_of<database>())
      l_pre.params.user_ref = l_h.get<database>().get_id();
    l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    auto l_r               = l_conn(l_pre);
    DOODLE_LOG_INFO("插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), entt::type_id<assets_file>().name());
  }
}

void sql_com<doodle::assets_file>::select(conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle) {
  auto& l_conn = *in_ptr;
  const tables::assets_file l_table{};
  const tables::entity l_entt_id{};
  std::vector<assets_file> l_assets;
  std::vector<entt::entity> l_entts;
  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
    l_assets.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  std::map<std::int64_t, entt::handle> l_user_h{};
  for (auto&& [e, data] : reg_->view<database>().each()) {
    l_user_h.emplace(data.get_id(), entt::handle{*reg_, e});
  }

  for (auto& row :
       l_conn(sqlpp::select(l_table.entity_id, l_table.name, l_table.path, l_table.version, l_table.user_ref)
                  .from(l_table)
                  .where(l_table.entity_id.is_not_null()))) {
    assets_file l_a{};
    l_a.name_attr(row.name.value());
    l_a.path_attr(row.path.value());
    l_a.version_attr(row.version.value());
    l_a.user_ref.handle_cache = l_user_h[row.user_ref.value()];

    auto l_id                 = row.entity_id.value();
    if (in_handle.find(l_id) != in_handle.end()) {
      l_assets.emplace_back(std::move(l_a));
      l_entts.emplace_back(in_handle.at(l_id));
      DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
    } else {
      DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
    }
  }
  reg_->insert<doodle::assets_file>(l_entts.begin(), l_entts.end(), l_assets.begin());
}
void sql_com<doodle::assets_file>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<tables::assets_file>(in_ptr, in_handle);
}

}  // namespace doodle::database_n