#include "work_task.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/logger/logger.h>

#include "sqlpp11/insert_value_list.h"
#include "sqlpp11/is_not_null.h"
#include "sqlpp11/sqlite3/connection.h"
#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
#include <utility>

namespace doodle::database_n {

void sql_com<doodle::work_task_info>::create_table(doodle::conn_ptr& in_ptr) {
  sql_create_table_base::create_table(in_ptr);
}

void sql_com<doodle::work_task_info>::insert(conn_ptr& in_ptr, const std::vector<entt::handle>& in_id) {
  auto& l_conn = *in_ptr;

  {
    // sql::WorkTaskInfo l_tabl{};
    tables::work_task_info l_tabl{};
    auto l_pre = l_conn.prepare(sqlpp::insert_into(l_tabl).set(
        l_tabl.ref_id = sqlpp::parameter(l_tabl.ref_id), l_tabl.task_name = sqlpp::parameter(l_tabl.task_name),
        l_tabl.region = sqlpp::parameter(l_tabl.region), l_tabl.abstract = sqlpp::parameter(l_tabl.abstract),
        l_tabl.time_point = sqlpp::parameter(l_tabl.time_point), l_tabl.entity_id = sqlpp::parameter(l_tabl.entity_id)
    ));

    for (auto& l_h : in_id) {
      auto& l_work            = l_h.get<doodle::work_task_info>();
      l_pre.params.region     = l_work.region;
      l_pre.params.task_name  = l_work.task_name;
      l_pre.params.abstract   = l_work.abstract;
      l_pre.params.entity_id  = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
      l_pre.params.time_point = l_work.time;
      if (auto l_user_h = l_work.user_ref.user_attr(); l_user_h.all_of<database>()) {
        l_pre.params.ref_id = l_user_h.get<database>().get_id();
      } else {
        l_pre.params.ref_id.set_null();
      }
      auto l_r = l_conn(l_pre);
      DOODLE_LOG_INFO("插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), entt::type_id<work_task_info>().name());
    }
  }
}
void sql_com<doodle::work_task_info>::update(conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id) {
  auto& l_conn = *in_ptr;

  {
    tables::work_task_info l_tabl{};

    auto l_pre = l_conn.prepare(
        sqlpp::update(l_tabl)
            .set(
                l_tabl.ref_id = sqlpp::parameter(l_tabl.ref_id), l_tabl.task_name = sqlpp::parameter(l_tabl.task_name),
                l_tabl.region = sqlpp::parameter(l_tabl.region), l_tabl.abstract = sqlpp::parameter(l_tabl.abstract),
                l_tabl.time_point = sqlpp::parameter(l_tabl.time_point)
            )
            .where(l_tabl.entity_id == sqlpp::parameter(l_tabl.entity_id))
    );

    for (const auto& [id, l_h] : in_id) {
      auto& l_work            = l_h.get<doodle::work_task_info>();
      l_pre.params.region     = l_work.region;
      l_pre.params.task_name  = l_work.task_name;
      l_pre.params.abstract   = l_work.abstract;
      l_pre.params.time_point = l_work.time;
      l_pre.params.entity_id  = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
      if (auto l_user_h = l_work.user_ref.user_attr(); l_user_h.all_of<database>()) {
        l_pre.params.ref_id = l_user_h.get<database>().get_id();
      } else {
        l_pre.params.ref_id.set_null();
      }
      auto l_r = l_conn(l_pre);
      DOODLE_LOG_INFO("更新数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), entt::type_id<work_task_info>().name());
    }
  }
}
void sql_com<doodle::work_task_info>::select(
    conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle, const registry_ptr& reg_
) {
  auto& l_conn = *in_ptr;

  {
    tables::work_task_info l_tabl{};
    std::vector<work_task_info> l_works{};
    std::vector<entt::entity> l_entts{};

    /// 选择大小并进行调整内存
    for (auto&& raw :
         l_conn(sqlpp::select(sqlpp::count(l_tabl.entity_id)).from(l_tabl).where(l_tabl.entity_id.is_not_null()))) {
      l_works.reserve(raw.count.value());
      l_entts.reserve(raw.count.value());
      break;
    }

    for (auto& row :
         l_conn(sqlpp::select(
                    l_tabl.entity_id, l_tabl.ref_id, l_tabl.task_name, l_tabl.region, l_tabl.abstract, l_tabl.time_point
         )
                    .from(l_tabl)
                    .where(l_tabl.entity_id.is_not_null()))) {
      work_task_info l_w{};
      l_w.abstract  = row.abstract.value();
      l_w.region    = row.region.value();

      l_w.time      = chrono_ns::round<doodle::work_task_info::time_point_type::duration>(row.time_point.value());
      l_w.task_name = row.task_name.value();
      if (!row.ref_id.is_null() && in_handle.count(row.ref_id.value()) != 0)
        l_w.user_ref.handle_cache = in_handle.at(row.ref_id.value());

      auto l_id = row.entity_id.value();
      if (in_handle.find(l_id) != in_handle.end()) {
        l_works.emplace_back(std::move(l_w));
        l_entts.emplace_back(in_handle.at(l_id));
        DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
      } else {
        DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
      }
    }
    reg_->insert<doodle::work_task_info>(l_entts.begin(), l_entts.end(), l_works.begin());
  }
}
void sql_com<doodle::work_task_info>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<tables::work_task_info>(in_ptr, in_handle);
}

}  // namespace doodle::database_n