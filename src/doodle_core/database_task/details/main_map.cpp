//
// Created by TD on 2023/11/21.
//
#include "main_map.h"

#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/main_map.h>
namespace doodle::database_n {

void sql_com<doodle::ue_main_map>::insert(conn_ptr &in_ptr, const std::vector<entt::handle> &in_id) {
  auto &l_conn = *in_ptr;

  tables::ue_main_map l_table{};

  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.map_path_ = sqlpp::parameter(l_table.map_path_), l_table.entity_id = sqlpp::parameter(l_table.entity_id)
  ));

  for (auto &l_h : in_id) {
    auto &l_ue             = l_h.get<ue_main_map>();
    l_pre.params.map_path_ = l_ue.project_path_.generic_string();
    l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    l_conn(l_pre);
  }
}

void sql_com<doodle::ue_main_map>::update(conn_ptr &in_ptr, const std::map<std::int64_t, entt::handle> &in_id) {
  auto &l_conn = *in_ptr;

  tables::ue_main_map l_table{};

  auto l_pre = l_conn.prepare(
      sqlpp::update(l_table)
          .set(l_table.map_path_ = sqlpp::parameter(l_table.map_path_))
          .where(l_table.entity_id == sqlpp::parameter(l_table.entity_id) && l_table.id == sqlpp::parameter(l_table.id))
  );
  for (auto &[id, l_h] : in_id) {
    auto &l_ue             = l_h.get<ue_main_map>();
    l_pre.params.map_path_ = l_ue.project_path_.generic_string();
    l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    l_pre.params.id        = id;
    l_conn(l_pre);
  }
}
void sql_com<doodle::ue_main_map>::select(
    conn_ptr &in_ptr, const std::map<std::int64_t, entt::handle> &in_handle, entt::registry &in_reg
) {
  auto &l_conn = *in_ptr;
  tables::ue_main_map l_table{};
  std::vector<ue_main_map> l_ue;
  std::vector<entt::entity> l_entts;
  // 调整内存
  for (auto &&raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
    l_ue.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  for (auto &row :
       l_conn(sqlpp::select(l_table.entity_id, l_table.map_path_).from(l_table).where(l_table.entity_id.is_not_null())
       )) {
    ue_main_map l_i{};
    l_i.project_path_ = row.map_path_.value();
    auto l_id     = row.entity_id.value();
    if (in_handle.find(l_id) != in_handle.end()) {
      l_ue.emplace_back(std::move(l_i));
      l_entts.emplace_back(in_handle.at(l_id));
    } else {
      log_error(fmt::format("image_icon id {} not found", l_id));
    }
  }
  in_reg.insert<doodle::ue_main_map>(l_entts.begin(), l_entts.end(), l_ue.begin());
}
void sql_com<doodle::ue_main_map>::destroy(conn_ptr &in_ptr, const std::vector<std::int64_t> &in_handle) {
  detail::sql_com_destroy<tables::ue_main_map>(in_ptr, in_handle);
}

}  // namespace doodle::database_n