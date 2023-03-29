#include "user.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/generate/core/sql_sql.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/user.h>

#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::database_n {
namespace sql = doodle_database;

void sql_com<doodle::user>::insert(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;

  sql::Usertab l_tabl{};

  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_tabl).set(
      l_tabl.userName        = sqlpp::parameter(l_tabl.userName),
      l_tabl.permissionGroup = sqlpp::parameter(l_tabl.permissionGroup),
      l_tabl.entityId        = sqlpp::parameter(l_tabl.entityId)
  ));

  for (auto& l_h : l_handles) {
    auto& l_user                 = l_h.get<user>();
    l_pre.params.userName        = l_user.p_string_;
    l_pre.params.permissionGroup = enum_to_num(l_user.power);
    l_pre.params.entityId        = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());

    auto l_r                     = l_conn(l_pre);
    DOODLE_LOG_INFO("插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<user>().get_name());
  }
}

void sql_com<doodle::user>::update(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;

  sql::Usertab l_tabl{};

  auto l_pre = l_conn.prepare(sqlpp::update(l_tabl)
                                  .set(
                                      l_tabl.userName        = sqlpp::parameter(l_tabl.userName),
                                      l_tabl.permissionGroup = sqlpp::parameter(l_tabl.permissionGroup)
                                  )
                                  .where(l_tabl.entityId == sqlpp::parameter(l_tabl.entityId)));

  for (auto& l_h : l_handles) {
    auto& l_user                 = l_h.get<user>();
    l_pre.params.userName        = l_user.p_string_;
    l_pre.params.permissionGroup = enum_to_num(l_user.power);
    l_pre.params.entityId        = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());

    auto l_r                     = l_conn(l_pre);
    DOODLE_LOG_INFO("更新数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<user>().get_name());
  }
}

void sql_com<doodle::user>::select(conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;

  {
    sql::Usertab l_tabl{};
    std::vector<user> l_works{};
    std::vector<entt::entity> l_entts{};

    /// 选择大小并进行调整内存
    for (auto&& raw :
         l_conn(sqlpp::select(sqlpp::count(l_tabl.entityId)).from(l_tabl).where(l_tabl.entityId.is_not_null()))) {
      l_works.reserve(raw.count.value());
      l_entts.reserve(raw.count.value());
      break;
    }

    for (auto& row : l_conn(sqlpp::select(l_tabl.entityId, l_tabl.userName, l_tabl.permissionGroup)
                                .from(l_tabl)
                                .where(l_tabl.entityId.is_null()))) {
      user l_u{};
      l_u.p_string_ = row.userName.value();
      l_u.power     = num_to_enum<power_enum>(row.permissionGroup.value());

      auto l_id     = row.entityId.value();
      if (in_handle.find(l_id) != in_handle.end()) {
        l_works.emplace_back(std::move(l_u));
        l_entts.emplace_back(in_handle.at(l_id));
        DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
      } else {
        DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
      }
    }
    reg_->insert(l_entts.begin(), l_entts.end(), l_works.begin());
  }
}

void sql_com<doodle::user>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<sql::Usertab>(in_ptr, in_handle);
}
}  // namespace doodle::database_n
