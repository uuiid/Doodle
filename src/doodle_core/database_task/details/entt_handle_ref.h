//
// Created by td_main on 2023/11/13.
//

#pragma once
#include <doodle_core/database_task/details/tables.h>
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>
namespace doodle::database_n {

template <typename table_type, typename base_type>
class entt_handle_ref : public detail::sql_create_table_base<table_type> {
 public:
  entt_handle_ref() = default;
  void insert(conn_ptr& in_ptr, const std::vector<entt::handle>& in_id);
  void update(conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id);
  void select(conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle, entt::registry& in_reg);
  void destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle);
};

template <typename table_type, typename base_type>
void entt_handle_ref<table_type, base_type>::insert(conn_ptr& in_ptr, const std::vector<entt::handle>& in_id) {
  auto& l_conn = *in_ptr;

  table_type l_table{};
  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.entity_id = sqlpp::parameter(l_table.entity_id), l_table.ref_id = sqlpp::parameter(l_table.ref_id)
  ));

  for (const auto& l_h : in_id) {
    auto& l_ref = l_h.get<base_type>();
    if (l_ref.template any_of<database>()) {
      l_pre.params.ref_id = l_ref.template get<database>().get_id();
    } else {
      continue;
    }
    l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    l_conn(l_pre);
  }
}

template <typename table_type, typename base_type>
void entt_handle_ref<table_type, base_type>::update(
    conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id
) {
  auto& l_conn = *in_ptr;
  table_type l_table{};
  std::vector<std::int64_t> l_ids;

  auto l_pre = l_conn.prepare(
      sqlpp::update(l_table)
          .set(l_table.ref_id = sqlpp::parameter(l_table.ref_id))
          .where(l_table.entity_id == sqlpp::parameter(l_table.entity_id) && l_table.id == sqlpp::parameter(l_table.id))
  );
  for (const auto& [id, l_h] : in_id) {
    auto& l_ref     = l_h.get<base_type>();
    l_pre.params.id = id;
    if (l_ref && l_ref.template any_of<database>())
      l_pre.params.ref_id = l_ref.template get<database>().get_id();
    else {
      l_ids.emplace_back(boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id()));
      continue;
    }

    l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    l_conn(l_pre);
  }
  destroy(in_ptr, l_ids);
}

template <typename table_type, typename base_type>
void entt_handle_ref<table_type, base_type>::select(
    conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle, entt::registry& in_reg
) {
  auto& l_conn = *in_ptr;
  const table_type l_table{};
  const tables::entity l_entt_id{};
  std::vector<base_type> l_refs;
  std::vector<entt::entity> l_entts;
  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
    l_refs.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  for (auto& row :
       l_conn(sqlpp::select(l_table.entity_id, l_table.ref_id).from(l_table).where(l_table.entity_id.is_not_null()))) {
    if (row.ref_id.is_null()) continue;

    auto l_ref = row.ref_id.value();
    auto l_id  = row.entity_id.value();
    if (in_handle.contains(l_id) && in_handle.contains(l_ref)) {
      l_refs.emplace_back(in_reg, in_handle.at(l_ref).entity());
      l_entts.emplace_back(in_handle.at(l_id));
    } else {
      log_error(fmt::format("数据库中存在无效的引用 {} -> {}", l_id, l_ref));
    }
  }

  in_reg.insert<base_type>(l_entts.begin(), l_entts.end(), l_refs.begin());
}
template <typename table_type, typename base_type>
void entt_handle_ref<table_type, base_type>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<table_type>(in_ptr, in_handle);
}
template <>
struct sql_com<doodle::file_association_ref>
    : public entt_handle_ref<tables::file_association_ref, doodle::file_association_ref> {};

}  // namespace doodle::database_n
