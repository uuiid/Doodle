#include "project.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/generate/core/sql_sql.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/project.h>

#include <rttr/type.h>
#include <sqlpp11/parameter.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::database_n {
namespace sql = doodle_database;

void sql_com<doodle::project>::insert(conn_ptr& in_ptr, const entt::observer& in_observer) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_observer | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  sql::Project l_tabl{};
  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_tabl).set(        
      l_tabl.entityId  = sqlpp::parameter(l_tabl.entityId),
      l_tabl.pName     = sqlpp::parameter(l_tabl.pName),
      l_tabl.pEnStr    = sqlpp::parameter(l_tabl.pEnStr),
      l_tabl.pPath     = sqlpp::parameter(l_tabl.pPath),
      l_tabl.pShorStr  = sqlpp::parameter(l_tabl.pShorStr)
  ));

  for (auto& l_h : l_handles) {
    auto&  l_project      = l_h.get<project>();
    l_pre.params.pEnStr   = l_project.p_en_str;
    l_pre.params.pName    = l_project.p_name;
    l_pre.params.pPath    = l_project.p_path.string();
    l_pre.params.pShorStr = l_project.p_shor_str;
    l_pre.params.entityId = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    
    auto l_r                     = l_conn(l_pre);
    DOODLE_LOG_INFO("插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(),rttr::type::get<project>().get_name());
  }
}

void sql_com<doodle::project>::update(conn_ptr& in_ptr, const entt::observer& in_observer) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_observer | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;

  sql::Project l_tabl{};

  auto l_pre = l_conn.prepare(sqlpp::update(l_tabl)
                                  .set(
                                      l_tabl.pName        = sqlpp::parameter(l_tabl.pName),
                                      l_tabl.pEnStr       = sqlpp::parameter(l_tabl.pEnStr),
                                      l_tabl.pPath        = sqlpp::parameter(l_tabl.pPath),
                                      l_tabl.pShorStr     = sqlpp::parameter(l_tabl.pShorStr)
                                  )
                                  .where(l_tabl.entityId == sqlpp::parameter(l_tabl.entityId)));

  for (auto& l_h : l_handles) {
    auto& l_project       = l_h.get<project>();
    l_pre.params.pName    = l_project.p_name;
    l_pre.params.pEnStr   = l_project.p_en_str;
    l_pre.params.pPath    = l_project.p_path.string();
    l_pre.params.pShorStr = l_project.p_shor_str;
    l_pre.params.entityId = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());

    auto l_r                     = l_conn(l_pre);
    DOODLE_LOG_INFO("更新数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<project>().get_name());
  }
}

void sql_com<doodle::project>::select(conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;

  {
    sql::Project l_tabl{};
    std::vector<project> l_works{};
    std::vector<entt::entity> l_entts{};

    /// 选择大小并进行调整内存
    for (auto&& raw :
         l_conn(sqlpp::select(sqlpp::count(l_tabl.entityId)).from(l_tabl).where(l_tabl.entityId.is_not_null()))) {
      l_works.reserve(raw.count.value());
      l_entts.reserve(raw.count.value());
      break;
    }

    for (auto& row : l_conn(sqlpp::select(l_tabl.entityId, l_tabl.pName, l_tabl.pEnStr, l_tabl.pPath, l_tabl.pShorStr)
                                .from(l_tabl)
                                .where(l_tabl.entityId.is_null()))) {
      project l_u{};
      l_u.p_name        = row.pName.value();
      l_u.p_en_str      = row.pEnStr.value();
      l_u.p_path        = row.pPath.value();
      l_u.p_shor_str    = row.pShorStr.value();
      auto l_id         = row.entityId.value();

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

void sql_com<doodle::project>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<sql::Project>(in_ptr, in_handle);
}

}  // namespace doodle::database_n
