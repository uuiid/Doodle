//
// Created by td_main on 2023/7/12.
//

#pragma once
#include <doodle_core/database_task/details/tables.h>
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/file_one_path.h>
namespace doodle::database_n {

template <typename table_type, typename base_type>
class file_one_path : public detail::sql_create_table_base<table_type> {
 public:
  file_one_path() = default;
  void insert(const sql_connection_ptr& in_ptr, const std::vector<entt::handle>& in_id) {
    auto& l_conn = *in_ptr;

    table_type l_table{};
    auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
        l_table.entity_id = sqlpp::parameter(l_table.entity_id), l_table.path = sqlpp::parameter(l_table.path)
    ));

    for (const auto& l_h : in_id) {
      auto& l_file           = l_h.get<base_type>();
      l_pre.params.path      = l_file.path_.generic_string();
      l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
      l_conn(l_pre);
      // DOODLE_LOG_INFO("插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), entt::type_id<base_type>().name());
    }
  };
  void update(const sql_connection_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id) {
    auto& l_conn = *in_ptr;
    table_type l_table{};

    auto l_pre = l_conn.prepare(sqlpp::update(l_table)
                                    .set(l_table.path = sqlpp::parameter(l_table.path))
                                    .where(
                                        l_table.entity_id == sqlpp::parameter(l_table.entity_id) &&
                                        l_table.id == sqlpp::parameter(l_table.id)
                                    ));
    for (const auto& [id, l_h] : in_id) {
      auto& l_assets         = l_h.get<base_type>();
      l_pre.params.id        = id;
      l_pre.params.path      = l_assets.path_.generic_string();
      l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
      l_conn(l_pre);
    }
  };
  void select(const sql_connection_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle, entt::registry& in_reg) {
    auto& l_conn = *in_ptr;
    const table_type l_table{};
    const tables::entity l_entt_id{};
    std::vector<base_type> l_assets;
    std::vector<entt::entity> l_entts;
    // 调整内存
    for (auto&& raw :
         l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
      l_assets.reserve(raw.count.value());
      l_entts.reserve(raw.count.value());
      break;
    }

    for (auto& row :
         l_conn(sqlpp::select(l_table.entity_id, l_table.path).from(l_table).where(l_table.entity_id.is_not_null()))) {
      base_type l_a{};
      l_a.path_ = row.path.value();

      auto l_id = row.entity_id.value();
      if (in_handle.find(l_id) != in_handle.end()) {
        l_assets.emplace_back(std::move(l_a));
        l_entts.emplace_back(in_handle.at(l_id));
        // DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
      } else {
        // DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
      }
    }

    in_reg.insert<base_type>(l_entts.begin(), l_entts.end(), l_assets.begin());
  };
  void destroy(const sql_connection_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
    detail::sql_com_destroy<table_type>(in_ptr, in_handle);
  };
};

}  // namespace doodle::database_n
