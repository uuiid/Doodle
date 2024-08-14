#include "project.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/project.h>

#include "sqlpp11/sqlite3/insert_or.h"
#include <entt/entity/fwd.hpp>
#include <sqlpp11/parameter.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::database_n {

void sql_com<doodle::project>::insert(conn_ptr& in_ptr, const std::vector<entt::handle>& in_id) {
  auto& l_conn = *in_ptr;

  tables::project l_tabl{};
  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_tabl).set(
      l_tabl.entity_id = sqlpp::parameter(l_tabl.entity_id), l_tabl.p_name = sqlpp::parameter(l_tabl.p_name),
      l_tabl.p_en_str = sqlpp::parameter(l_tabl.p_en_str), l_tabl.p_path = sqlpp::parameter(l_tabl.p_path),
      l_tabl.p_shor_str = sqlpp::parameter(l_tabl.p_shor_str)
  ));

  for (auto& l_h : in_id) {
    auto& l_project         = l_h.get<project>();
    l_pre.params.p_en_str   = l_project.p_en_str;
    l_pre.params.p_name     = l_project.p_name;
    l_pre.params.p_path     = l_project.p_path.string();
    l_pre.params.p_shor_str = l_project.p_shor_str;
    l_pre.params.entity_id  = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());

    auto l_r                = l_conn(l_pre);
    // DOODLE_LOG_INFO("插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), entt::type_id<project>().name());
  }
}

void sql_com<doodle::project>::update(conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id) {
  auto& l_conn = *in_ptr;

  tables::project l_tabl{};

  auto l_pre = l_conn.prepare(
      sqlpp::update(l_tabl)
          .set(
              l_tabl.p_name = sqlpp::parameter(l_tabl.p_name), l_tabl.p_en_str = sqlpp::parameter(l_tabl.p_en_str),
              l_tabl.p_path = sqlpp::parameter(l_tabl.p_path), l_tabl.p_shor_str = sqlpp::parameter(l_tabl.p_shor_str)
          )
          .where(l_tabl.entity_id == sqlpp::parameter(l_tabl.entity_id) && l_tabl.id == sqlpp::parameter(l_tabl.id))
  );

  for (auto& [id, l_h] : in_id) {
    auto& l_project         = l_h.get<project>();
    l_pre.params.p_name     = l_project.p_name;
    l_pre.params.p_en_str   = l_project.p_en_str;
    l_pre.params.p_path     = l_project.p_path.string();
    l_pre.params.p_shor_str = l_project.p_shor_str;
    l_pre.params.entity_id  = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    l_pre.params.id         = id;

    auto l_r                = l_conn(l_pre);
    // DOODLE_LOG_INFO("更新数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), entt::type_id<project>().name());
  }
}

void sql_com<doodle::project>::select(
    conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle, entt::registry& in_reg
) {
  auto& l_conn = *in_ptr;

  {
    tables::project l_tabl{};
    std::vector<project> l_works{};
    std::vector<entt::entity> l_entts{};

    /// 选择大小并进行调整内存
    for (auto&& raw :
         l_conn(sqlpp::select(sqlpp::count(l_tabl.entity_id)).from(l_tabl).where(l_tabl.entity_id.is_not_null()))) {
      l_works.reserve(raw.count.value());
      l_entts.reserve(raw.count.value());
      break;
    }

    for (auto& row :
         l_conn(sqlpp::select(l_tabl.entity_id, l_tabl.p_name, l_tabl.p_en_str, l_tabl.p_path, l_tabl.p_shor_str)
                    .from(l_tabl)
                    .where(l_tabl.entity_id.is_not_null()))) {
      project l_u{};
      l_u.p_name     = row.p_name.value();
      l_u.p_en_str   = row.p_en_str.value();
      l_u.p_path     = row.p_path.value();
      l_u.p_shor_str = row.p_shor_str.value();
      auto l_id      = row.entity_id.value();

      if (in_handle.find(l_id) != in_handle.end()) {
        l_works.emplace_back(std::move(l_u));
        l_entts.emplace_back(in_handle.at(l_id));
        // DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
      } else {
        // DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
      }
    }
    in_reg.insert<doodle::project>(l_entts.begin(), l_entts.end(), l_works.begin());
  }
}

void sql_com<doodle::project>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<tables::project>(in_ptr, in_handle);
}

void sql_ctx<project>::insert(conn_ptr& in_ptr, const project& in_id) {
  auto& l_conn = *in_ptr;

  tables::project const l_tabl{};
  auto l_select = l_conn(sqlpp::select(l_tabl.id).from(l_tabl).unconditionally());
  auto id       = l_select.empty() ? 0 : l_select.front().id.value();

  l_conn(sqlpp::sqlite3::insert_or_replace_into(l_tabl).set(
      l_tabl.id = id, l_tabl.p_name = in_id.p_name, l_tabl.p_en_str = in_id.p_en_str,
      l_tabl.p_path = in_id.p_path.generic_string(), l_tabl.p_shor_str = in_id.p_shor_str
  ));

  // DOODLE_LOG_INFO("插入数据库组件 {} ", entt::type_id<project>().name());
}

void sql_ctx<project>::select(conn_ptr& in_ptr, project& in_handle) {
  auto& l_conn = *in_ptr;

  {
    tables::project l_tabl{};

    for (auto& row :
         l_conn(sqlpp::select(l_tabl.entity_id, l_tabl.p_name, l_tabl.p_en_str, l_tabl.p_path, l_tabl.p_shor_str)
                    .from(l_tabl)
                    .where(l_tabl.entity_id.is_not_null()))) {
      in_handle.p_name     = row.p_name.value();
      in_handle.p_en_str   = row.p_en_str.value();
      in_handle.p_path     = row.p_path.value();
      in_handle.p_shor_str = row.p_shor_str.value();
      auto l_id            = row.entity_id.value();
      // DOODLE_LOG_INFO("选择数据库 id {} 插入上下文", l_id);
      break;
    }
  }
}

}  // namespace doodle::database_n
