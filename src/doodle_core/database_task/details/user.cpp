#include "user.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/user.h>

#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::database_n {

void sql_com<doodle::user>::insert(const sql_connection_ptr& in_ptr, const std::vector<entt::handle>& in_id) {
  auto& l_conn = *in_ptr;

  tables::usertab l_tabl{};

  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_tabl).set(
      l_tabl.user_name        = sqlpp::parameter(l_tabl.user_name),
      l_tabl.permission_group = sqlpp::parameter(l_tabl.permission_group),
      l_tabl.entity_id        = sqlpp::parameter(l_tabl.entity_id)
  ));

  for (auto& l_h : in_id) {
    auto& l_user                  = l_h.get<user>();
    l_pre.params.user_name        = l_user.p_string_;
    l_pre.params.permission_group = enum_to_num(l_user.power);
    l_pre.params.entity_id        = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());

    auto l_r                      = l_conn(l_pre);
    // DOODLE_LOG_INFO("插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), entt::type_id<user>().name());
  }
}

void sql_com<doodle::user>::update(const sql_connection_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id) {
  auto& l_conn = *in_ptr;

  tables::usertab l_tabl{};

  auto l_pre = l_conn.prepare(
      sqlpp::update(l_tabl)
          .set(
              l_tabl.user_name        = sqlpp::parameter(l_tabl.user_name),
              l_tabl.permission_group = sqlpp::parameter(l_tabl.permission_group)
          )
          .where(l_tabl.entity_id == sqlpp::parameter(l_tabl.entity_id) && l_tabl.id == sqlpp::parameter(l_tabl.id))
  );

  for (auto& [id, l_h] : in_id) {
    auto& l_user                  = l_h.get<user>();
    l_pre.params.user_name        = l_user.p_string_;
    l_pre.params.permission_group = enum_to_num(l_user.power);
    l_pre.params.entity_id        = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    l_pre.params.id               = id;

    auto l_r                      = l_conn(l_pre);
    // DOODLE_LOG_INFO("更新数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), entt::type_id<user>().name());
  }
}

void sql_com<doodle::user>::select(
    const sql_connection_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle, entt::registry& reg_
) {
  auto& l_conn = *in_ptr;

  {
    tables::usertab l_tabl{};
    std::vector<user> l_works{};
    std::vector<entt::entity> l_entts{};

    /// 选择大小并进行调整内存
    for (auto&& raw :
         l_conn(sqlpp::select(sqlpp::count(l_tabl.entity_id)).from(l_tabl).where(l_tabl.entity_id.is_not_null()))) {
      l_works.reserve(raw.count.value());
      l_entts.reserve(raw.count.value());
      break;
    }

    for (auto& row : l_conn(sqlpp::select(l_tabl.entity_id, l_tabl.user_name, l_tabl.permission_group)
                                .from(l_tabl)
                                .where(l_tabl.entity_id.is_not_null()))) {
      user l_u{};
      l_u.p_string_ = row.user_name.value();
      l_u.power     = num_to_enum<power_enum>(row.permission_group.value());

      auto l_id     = row.entity_id.value();
      if (in_handle.find(l_id) != in_handle.end()) {
        l_works.emplace_back(std::move(l_u));
        l_entts.emplace_back(in_handle.at(l_id));
        // DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
      } else {
        // DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
      }
    }
    reg_.insert<doodle::user>(l_entts.begin(), l_entts.end(), l_works.begin());
  }
}

void sql_com<doodle::user>::destroy(const sql_connection_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<tables::usertab>(in_ptr, in_handle);
}
}  // namespace doodle::database_n
