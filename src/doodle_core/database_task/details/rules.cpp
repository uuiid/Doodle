#include "rules.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/logger/logger.h>

#include "metadata/metadata.h"
#include "metadata/rules.h"
#include "sqlpp11/data_types/time_point/data_type.h"
#include "sqlpp11/insert.h"
#include "sqlpp11/parameter.h"
#include "sqlpp11/select.h"
#include <__msvc_chrono.hpp>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <lib_warp/enum_template_tool.h>
#include <magic_enum.hpp>
#include <map>
#include <sqlpp11/aggregate_functions/count.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
#include <string>
#include <vector>

namespace doodle::database_n {

void sql_com<doodle::business::rules>::insert_sub(
    conn_ptr& in_ptr, const std::vector<entt::handle>& in_handles, const std::map<entt::handle, std::int64_t>& in_map
) {
  auto& l_conn = *in_ptr;
  {
    const tables::business_rules_work_pair l_table{};
    auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
        l_table.parent_id           = sqlpp::parameter(l_table.parent_id),
        l_table.first_time_seconds  = sqlpp::parameter(l_table.first_time_seconds),
        l_table.second_time_seconds = sqlpp::parameter(l_table.second_time_seconds)
    ));

    for (auto& l_h : in_handles) {
      auto& l_r = l_h.get<business::rules>();
      for (auto&& l_work : l_r.work_pair_p) {
        l_pre.params.parent_id           = in_map.at(l_h);
        l_pre.params.first_time_seconds  = l_work.first.count();
        l_pre.params.second_time_seconds = l_work.second.count();
        l_conn(l_pre);
      }
    }
  }

  {
    const tables::business_rules_work_abs_pair l_table{};
    auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
        l_table.parent_id           = sqlpp::parameter(l_table.parent_id),
        l_table.work_index          = sqlpp::parameter(l_table.work_index),
        l_table.first_time_seconds  = sqlpp::parameter(l_table.first_time_seconds),
        l_table.second_time_seconds = sqlpp::parameter(l_table.second_time_seconds)
    ));
    for (auto& l_h : in_handles) {
      auto& l_r = l_h.get<business::rules>();
      for (auto i = 0; i < l_r.absolute_deduction.size(); ++i) {
        for (auto&& l_work : l_r.absolute_deduction[i]) {
          l_pre.params.parent_id           = in_map.at(l_h);
          l_pre.params.work_index          = i;
          l_pre.params.first_time_seconds  = l_work.first.count();
          l_pre.params.second_time_seconds = l_work.second.count();
          l_conn(l_pre);
        }
      }
    }
  }

  {
    const tables::business_rules_time_info_time_info l_table{};
    auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
        l_table.parent_id   = sqlpp::parameter(l_table.parent_id),
        l_table.first_time  = sqlpp::parameter(l_table.first_time),
        l_table.second_time = sqlpp::parameter(l_table.second_time), l_table.info = sqlpp::parameter(l_table.info),
        l_table.is_extra_work = sqlpp::parameter(l_table.is_extra_work)
    ));
    for (auto& l_h : in_handles) {
      auto& l_r = l_h.get<business::rules>();
      for (auto&& l_work : l_r.extra_p) {
        l_pre.params.parent_id = in_map.at(l_h);
        l_pre.params.first_time =
            chrono_ns::round<sqlpp::time_point::_cpp_value_type::duration>(l_work.first.get_sys_time());
        l_pre.params.second_time =
            chrono_ns::round<sqlpp::time_point::_cpp_value_type::duration>(l_work.second.get_sys_time());
        l_pre.params.info          = l_work.info;
        l_pre.params.is_extra_work = l_work.is_extra_work;
        l_conn(l_pre);
      }
    }
  }
}

void sql_com<doodle::business::rules>::insert(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  auto& l_conn   = *in_ptr;
  auto l_handles = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  tables::business_rules l_table{};
  std::map<entt::handle, std::int64_t> l_map_id{};

  {
    auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
        l_table.work_weekdays = sqlpp::parameter(l_table.work_weekdays),
        l_table.entity_id     = sqlpp::parameter(l_table.entity_id)
    ));

    for (auto& l_h : l_handles) {
      auto& l_rules              = l_h.get<business::rules>();
      l_pre.params.work_weekdays = l_rules.work_weekdays_p.to_string();
      l_pre.params.entity_id     = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
      auto l_r                   = l_conn(l_pre);
      l_map_id.emplace(l_h, l_r);
      DOODLE_LOG_INFO(
          "插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), entt::type_id<business::rules>().name()
      );
    }
  }
  insert_sub(in_ptr, l_handles, l_map_id);
}

void sql_com<doodle::business::rules>::update(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  auto& l_conn   = *in_ptr;
  auto l_handles = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  tables::business_rules l_table{};

  {
    auto l_pre = l_conn.prepare(sqlpp::update(l_table)
                                    .set(l_table.work_weekdays = sqlpp::parameter(l_table.work_weekdays))
                                    .where(l_table.entity_id == sqlpp::parameter(l_table.entity_id)));
    for (auto& l_h : l_handles) {
      auto& l_rules              = l_h.get<business::rules>();
      l_pre.params.work_weekdays = l_rules.work_weekdays_p.to_string();
      l_pre.params.entity_id     = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());

      auto l_r                   = l_conn(l_pre);
      DOODLE_LOG_INFO(
          "更新数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), entt::type_id<business::rules>().name()
      );
    }
  }

  auto l_map_id = detail::sql_com_destroy_parent_id_return_id<
      tables::business_rules, tables::business_rules_work_pair, tables::business_rules_work_abs_pair,
      tables::business_rules_time_info_time_info>(in_ptr, l_handles);

  insert_sub(in_ptr, l_handles, l_map_id);
}

void sql_com<doodle::business::rules>::select(conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle) {
  auto& l_conn = *in_ptr;
  std::vector<business::rules> l_assets;
  std::vector<entt::entity> l_entts;
  std::map<entt::entity, std::int64_t> l_map_id{};
  {
    const tables::business_rules l_table{};
    // 调整内存
    for (auto&& raw :
         l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
      l_assets.reserve(raw.count.value());
      l_entts.reserve(raw.count.value());
      break;
    }
    {
      for (auto& row :
           l_conn(sqlpp::select(l_table.id, l_table.entity_id, l_table.work_weekdays).from(l_table).unconditionally()
           )) {
        business::rules l_s{};
        l_s.work_weekdays_p = decltype(l_s.work_weekdays_p){row.work_weekdays.value()};
        auto l_id           = row.entity_id.value();
        if (in_handle.find(l_id) != in_handle.end()) {
          l_assets.emplace_back(std::move(l_s));
          l_entts.emplace_back(in_handle.at(l_id));
          l_map_id.emplace(in_handle.at(l_id), row.id.value());
          DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
        } else {
          DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
        }
      }
    }
  }

  {
    const tables::business_rules_work_pair l_table{};

    auto l_pre = l_conn.prepare(sqlpp::select(l_table.first_time_seconds, l_table.second_time_seconds)
                                    .from(l_table)
                                    .where(l_table.parent_id == sqlpp::parameter(l_table.parent_id)));
    for (auto i = 0; i < l_assets.size(); ++i) {
      l_pre.params.parent_id = l_map_id.at(l_entts[i]);
      for (auto&& row : l_conn(l_pre)) {
        l_assets[i].work_pair_p.emplace_back(
            business::rules::duration_type{row.first_time_seconds.value()},
            business::rules::duration_type{row.second_time_seconds.value()}
        );
      }
    }
  }
  {
    const tables::business_rules_work_abs_pair l_table{};

    auto l_pre =
        l_conn.prepare(sqlpp::select(l_table.first_time_seconds, l_table.second_time_seconds, l_table.work_index)
                           .from(l_table)
                           .where(l_table.parent_id == sqlpp::parameter(l_table.parent_id)));
    for (auto i = 0; i < l_assets.size(); ++i) {
      l_pre.params.parent_id = l_map_id.at(l_entts[i]);
      for (auto&& row : l_conn(l_pre)) {
        l_assets[i].absolute_deduction[row.work_index.value()].emplace_back(
            row.first_time_seconds.value(), row.second_time_seconds.value()
        );
      }
    }
  }

  {
    const tables::business_rules_time_info_time_info l_table{};

    auto l_pre =
        l_conn.prepare(sqlpp::select(l_table.first_time, l_table.second_time, l_table.info, l_table.is_extra_work)
                           .from(l_table)
                           .where(l_table.parent_id == sqlpp::parameter(l_table.parent_id)));

    for (auto i = 0; i < l_assets.size(); ++i) {
      l_pre.params.parent_id = l_map_id.at(l_entts[i]);
      for (auto&& row : l_conn(l_pre)) {
        l_assets[i].extra_p.emplace_back(
            row.first_time.value(), row.second_time.value(), row.info.value(), row.is_extra_work.value()
        );
      }
    }
  }

  reg_->insert(l_entts.begin(), l_entts.end(), l_assets.begin());
}
void sql_com<doodle::business::rules>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<tables::assets>(in_ptr, in_handle);
  detail::sql_com_destroy_parent_id<tables::business_rules_work_pair>(in_ptr, in_handle);
  detail::sql_com_destroy_parent_id<tables::business_rules_work_abs_pair>(in_ptr, in_handle);
  detail::sql_com_destroy_parent_id<tables::business_rules_time_info_time_info>(in_ptr, in_handle);
}

}  // namespace doodle::database_n