#include "time_point_info.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/generate/core/sql_sql.h>
#include <doodle_core/logger/logger.h>

#include <boost/locale/date_time.hpp>

#include "metadata/detail/time_point_info.h"
#include "metadata/metadata.h"
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
namespace sql = doodle_database;
void sql_com<doodle::business::rules_ns::time_point_info>::insert(conn_ptr& in_ptr, const entt::observer& in_observer) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_observer | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  sql::TimePointInfo l_table{};

  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.firstTime  = sqlpp::parameter(l_table.firstTime),
      l_table.secondTime = sqlpp::parameter(l_table.secondTime), l_table.info = sqlpp::parameter(l_table.info),
      l_table.isExtraWork = sqlpp::parameter(l_table.isExtraWork), l_table.entityId = sqlpp::parameter(l_table.entityId)
  ));

  for (auto& l_h : l_handles) {
    auto& l_time             = l_h.get<doodle::business::rules_ns::time_point_info>();
    // todo:时间转换
    //  l_pre.params.firstTime  = l_time.first.get_local_time();
    //  l_pre.params.secondTime=l_time.second;
    l_pre.params.info        = l_time.info;
    l_pre.params.isExtraWork = l_time.is_extra_work;
    l_pre.params.entityId    = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    auto l_r                 = l_conn(l_pre);
    DOODLE_LOG_INFO(
        "插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(),
        rttr::type::get<business::rules_ns::time_point_info>().get_name()
    );
  }
}

void sql_com<doodle::business::rules_ns::time_point_info>::update(conn_ptr& in_ptr, const entt::observer& in_observer) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_observer | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  sql::TimePointInfo l_table{};

  auto l_pre = l_conn.prepare(sqlpp::update(l_table)
                                  .set(
                                      l_table.firstTime   = sqlpp::parameter(l_table.firstTime),
                                      l_table.secondTime  = sqlpp::parameter(l_table.secondTime),
                                      l_table.info        = sqlpp::parameter(l_table.info),
                                      l_table.isExtraWork = sqlpp::parameter(l_table.isExtraWork)
                                  )
                                  .where(l_table.entityId == sqlpp::parameter(l_table.entityId)));
  for (auto& l_h : l_handles) {
    auto& l_time             = l_h.get<doodle::business::rules_ns::time_point_info>();
    // todo:时间转换
    //  l_pre.params.firstTime  = l_time.first.get_local_time();
    //  l_pre.params.secondTime=l_time.second;
    l_pre.params.info        = l_time.info;
    l_pre.params.isExtraWork = l_time.is_extra_work;
    l_pre.params.entityId    = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    auto l_r                 = l_conn(l_pre);

    DOODLE_LOG_INFO(
        "更新数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(),
        rttr::type::get<business::rules_ns::time_point_info>().get_name()
    );
  }
}
void sql_com<doodle::business::rules_ns::time_point_info>::select(
    conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle
) {
  auto& l_conn = *in_ptr;
  sql::TimePointInfo l_table{};
  std::vector<business::rules_ns::time_point_info> l_time;
  std::vector<entt::entity> l_entts;
  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entityId)).from(l_table).where(l_table.entityId.is_not_null()))) {
       l_time.reserve(raw.count.value());
       l_entts.reserve(raw.count.value());
       break;
  }

  for (auto& row : l_conn(sqlpp::select(l_table.entityId, l_table.firstTime, l_table.secondTime,l_table.info,l_table.isExtraWork)
                              .from(l_table)
                              .where(l_table.entityId.is_null()))) {
    business::rules_ns::time_point_info l_t{};
    l_t.first=row.firstTime.value();
    l_t.second=row.secondTime.value();
    l_t.info=row.info.value();
    l_t.is_extra_work=row.isExtraWork.value();
    auto l_id       = row.entityId.value();
    if (in_handle.find(l_id) != in_handle.end()) {
      l_time.emplace_back(std::move(l_t));
      l_entts.emplace_back(in_handle.at(l_id));
      DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
    } else {
      DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
    }
  }
  reg_->insert(l_entts.begin(), l_entts.end(), l_time.begin());
}
void sql_com<doodle::business::rules_ns::time_point_info>::destroy(
    conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle
) {
  detail::sql_com_destroy<sql::TimePointInfo>(in_ptr, in_handle);
}

}  // namespace doodle::database_n