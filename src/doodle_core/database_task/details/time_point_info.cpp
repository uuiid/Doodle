#include "time_point_info.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/logger/logger.h>

#include <boost/locale/date_time.hpp>

#include "metadata/detail/time_point_info.h"
#include "metadata/metadata.h"
#include "metadata/time_point_wrap.h"
#include <algorithm>
#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <lib_warp/enum_template_tool.h>
#include <magic_enum.hpp>
#include <sqlpp11/aggregate_functions/count.h>
#include <sqlpp11/data_types/time_point/data_type.h>
#include <sqlpp11/parameter.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::database_n {
void sql_com<doodle::business::rules_ns::time_point_info>::insert(
    conn_ptr& in_ptr, const std::vector<entt::handle>& in_id
) {
  auto& l_conn = *in_ptr;

  tables::time_point_info l_table{};

  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.first_time  = sqlpp::parameter(l_table.first_time),
      l_table.second_time = sqlpp::parameter(l_table.second_time), l_table.info = sqlpp::parameter(l_table.info),
      l_table.is_extra_work = sqlpp::parameter(l_table.is_extra_work),
      l_table.entity_id     = sqlpp::parameter(l_table.entity_id)
  ));

  for (auto& l_h : in_id) {
    auto& l_time               = l_h.get<doodle::business::rules_ns::time_point_info>();
    l_pre.params.first_time    = chrono_ns::round<chrono_ns::microseconds>(l_time.first.get_sys_time());
    l_pre.params.second_time   = chrono_ns::round<chrono_ns::microseconds>(l_time.second.get_sys_time());
    l_pre.params.info          = l_time.info;
    l_pre.params.is_extra_work = l_time.is_extra_work;
    l_pre.params.entity_id     = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    auto l_r                   = l_conn(l_pre);
    DOODLE_LOG_INFO(
        "插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(),
        entt::type_id<business::rules_ns::time_point_info>().name()
    );
  }
}

void sql_com<doodle::business::rules_ns::time_point_info>::update(
    conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id
) {
  auto& l_conn = *in_ptr;

  tables::time_point_info l_table{};

  auto l_pre = l_conn.prepare(
      sqlpp::update(l_table)
          .set(
              l_table.first_time    = sqlpp::parameter(l_table.first_time),
              l_table.second_time   = sqlpp::parameter(l_table.second_time),
              l_table.info          = sqlpp::parameter(l_table.info),
              l_table.is_extra_work = sqlpp::parameter(l_table.is_extra_work)
          )
          .where(l_table.entity_id == sqlpp::parameter(l_table.entity_id) && l_table.id == sqlpp::parameter(l_table.id))
  );
  for (const auto& [id, l_h] : in_id) {
    auto& l_time               = l_h.get<doodle::business::rules_ns::time_point_info>();

    l_pre.params.first_time    = chrono_ns::round<chrono_ns::microseconds>(l_time.first.get_sys_time());
    l_pre.params.second_time   = chrono_ns::round<chrono_ns::microseconds>(l_time.second.get_sys_time());
    l_pre.params.info          = l_time.info;
    l_pre.params.is_extra_work = l_time.is_extra_work;
    l_pre.params.entity_id     = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    l_pre.params.id            = id;

    auto l_r                   = l_conn(l_pre);
    DOODLE_LOG_INFO(
        "更新数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(),
        entt::type_id<business::rules_ns::time_point_info>().name()
    );
  }
}
void sql_com<doodle::business::rules_ns::time_point_info>::select(
    conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle,  const registry_ptr& reg_
) {
  auto& l_conn = *in_ptr;
  tables::time_point_info l_table{};
  std::vector<business::rules_ns::time_point_info> l_time;
  std::vector<entt::entity> l_entts;
  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
    l_time.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  for (auto& row :
       l_conn(sqlpp::select(
                  l_table.entity_id, l_table.first_time, l_table.second_time, l_table.info, l_table.is_extra_work
       )
                  .from(l_table)
                  .where(l_table.entity_id.is_not_null()))) {
    business::rules_ns::time_point_info l_t{};
    l_t.first         = row.first_time.value();
    l_t.second        = row.second_time.value();
    l_t.info          = row.info.value();
    l_t.is_extra_work = row.is_extra_work.value();
    auto l_id         = row.entity_id.value();
    if (in_handle.find(l_id) != in_handle.end()) {
      l_time.emplace_back(std::move(l_t));
      l_entts.emplace_back(in_handle.at(l_id));
      DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
    } else {
      DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
    }
  }
  reg_->insert<doodle::business::rules_ns::time_point_info>(l_entts.begin(), l_entts.end(), l_time.begin());
}
void sql_com<doodle::business::rules_ns::time_point_info>::destroy(
    conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle
) {
  detail::sql_com_destroy<tables::time_point_info>(in_ptr, in_handle);
}

}  // namespace doodle::database_n