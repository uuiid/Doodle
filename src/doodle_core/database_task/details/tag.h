//
// Created by TD on 2023/11/21.
//

#pragma once
#include <doodle_core/database_task/details/tables.h>
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/logger/logger.h>
namespace doodle::database_n {

/// tag 序列化
template <typename tag_type>
class tag_serialization : public detail::sql_create_table_base<tables::tag_table> {
 public:
  tag_serialization()  = default;
  ~tag_serialization() = default;

  void insert(const sql_connection_ptr& in_ptr, const std::vector<entt::handle>& in_id) {
    auto& l_conn = *in_ptr;

    const tables::tag_table l_table{};
    auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
        l_table.entity_id = sqlpp::parameter(l_table.entity_id), l_table.tag_id = tag_type{}()
    ));

    for (const auto& l_h : in_id) {
      l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
      l_conn(l_pre);
    }
  }
  void update(const sql_connection_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id) {
    //    auto& l_conn = *in_ptr;
    //    const tables::tag_table l_table{};
    //
    //    auto l_pre = l_conn.prepare(sqlpp::update(l_table)
    //                                    .set(l_table.tag_id = tag_type{}())
    //                                    .where(
    //                                        l_table.entity_id == sqlpp::parameter(l_table.entity_id) &&
    //                                        l_table.id == sqlpp::parameter(l_table.id)
    //                                    ));
    //    for (const auto& [id, l_h] : in_id) {
    //      l_pre.params.id        = id;
    //      l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    //      l_conn(l_pre);
    //    }
  }
  void select(const sql_connection_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle, entt::registry& in_reg) {
    auto& l_conn = *in_ptr;
    const tables::tag_table l_table{};
    const tables::entity l_entt_id{};
    std::vector<entt::entity> l_entts;
    // 调整内存
    for (auto&& raw : l_conn(sqlpp::select(sqlpp::count(l_table.entity_id))
                                 .from(l_table)
                                 .where(l_table.entity_id.is_not_null() && l_table.tag_id == tag_type{}()))) {
      l_entts.reserve(raw.count.value());
      break;
    }

    for (auto& row : l_conn(sqlpp::select(l_table.entity_id)
                                .from(l_table)
                                .where(l_table.entity_id.is_not_null() && l_table.tag_id == tag_type{}()))) {
      auto l_id = row.entity_id.value();
      if (in_handle.find(l_id) != in_handle.end()) {
        l_entts.emplace_back(in_handle.at(l_id));
      } else {
        log_error(fmt::format("数据库id {} 没有找到实体", l_id));
      }
    }
    in_reg.insert<tag_type>(l_entts.begin(), l_entts.end(), tag_type{});
  }
  void destroy(const sql_connection_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
    detail::sql_com_destroy<tables::tag_table>(in_ptr, in_handle);
  }
};

template <>
struct sql_com<doodle::main_project> : public tag_serialization<doodle::main_project> {};
template <>
struct sql_com<doodle::scene_id> : public tag_serialization<doodle::scene_id> {};
template <>
struct sql_com<doodle::character_id> : public tag_serialization<doodle::character_id> {};
template <>
struct sql_com<doodle::prop_id> : public tag_serialization<doodle::prop_id> {};
template <>
struct sql_com<doodle::rig_id> : public tag_serialization<doodle::rig_id> {};
template <>
struct sql_com<doodle::animation_id> : public tag_serialization<doodle::animation_id> {};
template <>
struct sql_com<doodle::simulation_id> : public tag_serialization<doodle::simulation_id> {};
}  // namespace doodle::database_n