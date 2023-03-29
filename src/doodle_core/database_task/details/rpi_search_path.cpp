#include "rpi_search_path.h"

#include "doodle_core_fwd.h"
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/generate/core/sql_sql.h>
#include <doodle_core/logger/logger.h>

#include <boost/filesystem/path.hpp>

#include "metadata/redirection_path_info.h"
#include <algorithm>
#include <entt/entity/fwd.hpp>
#include <lib_warp/enum_template_tool.h>
#include <magic_enum.hpp>
#include <sqlpp11/aggregate_functions/count.h>
#include <sqlpp11/insert.h>
#include <sqlpp11/parameter.h>
#include <sqlpp11/select.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/table_ref.h>
#include <vector>

namespace doodle::database_n {
namespace sql = doodle_database;
void sql_com<std::vector<FSys::path>>::insert(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_id) {
  auto& l_conn   = *in_ptr;
  auto l_handles = in_observer | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  sql::RpiSearchPath l_table{};
  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.path = sqlpp::parameter(l_table.path), l_table.entityId = sqlpp::parameter(l_table.entityId)
  ));

  for (auto& l_h : l_handles) {
    auto& l_path = l_h.get<redirection_path_info>().search_path_;
    for (auto& i : l_path) {
      l_pre.params.path = i.string();
      auto l_r          = l_conn(l_pre);
    }
  }
}

void sql_com<std::vector<FSys::path>>::update(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_id) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_observer | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  sql::RpiSearchPath l_table{};

  auto l_pre = l_conn.prepare(sqlpp::update(l_table)
                                  .set(l_table.path = sqlpp::parameter(l_table.path))
                                  .where(l_table.entityId == sqlpp::parameter(l_table.entityId)));
  for (auto& l_h : l_handles) {
    auto& l_path = l_h.get<redirection_path_info>().search_path_;
    for (auto& i : l_path) {
      l_pre.params.path = i.string();
      auto l_r          = l_conn(l_pre);
    }
  }
}

void sql_com<std::vector<FSys::path>>::select(conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle) {
  auto& l_conn = *in_ptr;
  sql::RpiSearchPath l_table{};
  std::vector<entt::entity> l_entts;
  std::vector<FSys::path> l_path;
  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entityId)).from(l_table).where(l_table.entityId.is_not_null()))) {
    l_path.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  for (auto& row :
       l_conn(sqlpp::select(l_table.entityId, l_table.path).from(l_table).where(l_table.entityId.is_null()))) {
    auto l_id = row.entityId.value();
    if (in_handle.find(l_id) != in_handle.end()) {
      l_path.emplace_back(row.path);
      l_entts.emplace_back(in_handle.at(l_id));
      DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
    } else {
      DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
    }
  }
  reg_->insert(l_entts.begin(), l_entts.end(), l_path.begin());
}
void sql_com<std::vector<FSys::path>>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<sql::RpiSearchPath>(in_ptr, in_handle);
}

}  // namespace doodle::database_n