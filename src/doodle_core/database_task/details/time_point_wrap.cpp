#include "time_point_wrap.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/logger/logger.h>

#include <boost/locale/date_time.hpp>

#include "metadata/detail/time_point_info.h"
#include "metadata/metadata.h"
#include "metadata/time_point_wrap.h"
#include <algorithm>
#include <entt/entity/fwd.hpp>
#include <lib_warp/enum_template_tool.h>
#include <magic_enum.hpp>
#include <sqlpp11/aggregate_functions/count.h>
#include <sqlpp11/data_types/time_point/data_type.h>
#include <sqlpp11/parameter.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::database_n {
void sql_com<doodle::time_point_wrap>::insert(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  auto& l_conn   = *in_ptr;
  auto l_handles = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  const tables::time_point_wrap l_table{};

  auto l_pre = l_conn.prepare(sqlpp::sqlite3::insert_or_replace_into(l_table).set(
      l_table.time_point = sqlpp::parameter(l_table.time_point), l_table.entity_id = sqlpp::parameter(l_table.entity_id)
  ));

  for (auto& l_h : l_handles) {
    auto& l_time = l_h.get<doodle::time_point_wrap>();
    l_pre.params.time_point =
        chrono_ns::time_point_cast<sqlpp::time_point::_cpp_value_type::duration>(l_time.get_sys_time());
    l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    auto l_r               = l_conn(l_pre);
    DOODLE_LOG_INFO("插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), entt::type_id<time_point_wrap>().name());
  }
}

void sql_com<doodle::time_point_wrap>::select(conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle) {
  auto& l_conn = *in_ptr;
  const tables::time_point_wrap l_table{};
  std::vector<time_point_wrap> l_time;
  std::vector<entt::entity> l_entts;
  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
    l_time.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  for (auto& row :
       l_conn(sqlpp::select(l_table.entity_id, l_table.time_point).from(l_table).where(l_table.entity_id.is_not_null())
       )) {
    auto l_id = row.entity_id.value();
    if (in_handle.find(l_id) != in_handle.end()) {
      l_time.emplace_back(row.time_point.value());
      l_entts.emplace_back(in_handle.at(l_id));
      DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
    } else {
      DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
    }
  }
  reg_->insert<doodle::time_point_wrap>(l_entts.begin(), l_entts.end(), l_time.begin());
}
void sql_com<doodle::time_point_wrap>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<tables::time_point_wrap>(in_ptr, in_handle);
}

}  // namespace doodle::database_n