//
// Created by TD on 2023/1/13.
//
#include "database.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/metadata.h>

#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::database_n {

void sql_com<doodle::database>::insert(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;

  tables::entity l_tabl{};

  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_tabl).set(l_tabl.uuid_data = sqlpp::parameter(l_tabl.uuid_data)));

  for (auto& l_h : l_handles) {
    auto& l_data           = l_h.get<database>();
    l_pre.params.uuid_data = uuids::to_string(l_data.uuid());
    auto l_r               = l_conn(l_pre);
    l_data.set_id(boost::numeric_cast<std::uint64_t>(l_r));
    DOODLE_LOG_INFO("插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<database>().get_name());
  }
}

void sql_com<doodle::database>::select(conn_ptr& in_ptr, std::map<std::int64_t, entt::entity>& in_handle) {
  auto& l_conn = *in_ptr;

  tables::entity l_tabl{};
  std::vector<database> l_data{};
  std::vector<std::int64_t> l_id{};
  std::vector<entt::entity> l_entts{};

  /// 选择大小并进行调整内存
  for (auto&& raw : l_conn(sqlpp::select(sqlpp::count(l_tabl.id)).from(l_tabl).unconditionally())) {
    l_data.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  for (auto& row : l_conn(sqlpp::select(l_tabl.id, l_tabl.uuid_data).from(l_tabl).unconditionally())) {
    database l_u{row.uuid_data.value()};
    l_u.set_id(row.id.value());

    l_data.emplace_back(l_u);
    l_id.emplace_back(row.id.value());
    l_entts.emplace_back(num_to_enum<entt::entity>(row.id.value()));
  }
  reg_->create(l_entts.begin(), l_entts.end());
  reg_->insert(l_entts.begin(), l_entts.end(), l_data.begin());

  for (auto i = 0; i < l_id.size(); ++i) in_handle.emplace(l_id[i], l_entts[i]);
}

void sql_com<doodle::database>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  auto& l_conn = *in_ptr;
  tables::entity l_tabl{};
  auto l_pre = l_conn.prepare(sqlpp::remove_from(l_tabl).where(l_tabl.id == sqlpp::parameter(l_tabl.id)));

  for (auto& l_id : in_handle) {
    l_pre.params.id = boost::numeric_cast<std::int64_t>(l_id);
    l_conn(l_pre);
  }
}
}  // namespace doodle::database_n
