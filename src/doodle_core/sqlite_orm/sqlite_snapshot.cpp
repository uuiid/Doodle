//
// Created by td_main on 2023/11/3.
//

#include "sqlite_snapshot.h"

#include <doodle_core/lib_warp/sqlite3/sqlite3.h>
#include <doodle_core/sqlite_orm/sqlite_base.h>

#include <sqlpp11/ppgen.h>
namespace doodle::snapshot {

// clang-format off

SQLPP_DECLARE_TABLE(
    (entity_tab),
    (id,                 int, SQLPP_PRIMARY_KEY)
    (entity_identifier,  int, SQLPP_NOT_NULL)
)

// clang-format on
namespace {
using entt_pie_sql_t = decltype(std::declval<sqlpp::sqlite3::connection>().prepare(
    sqlpp::sqlite3::insert_into(entity_tab::entity_tab{})
        .set(entity_tab::entity_tab{}.entity_identifier = sqlpp::parameter(entity_tab::entity_tab{}.entity_identifier))
        .on_conflict(entity_tab::entity_tab{}.entity_identifier)
        .do_nothing()
));

void create_table(entt::entity&, const conn_ptr& in_conn) {
  detail::sql_table_base<entity_tab::entity_tab> l_tab{};
  l_tab.create_table(in_conn);
}

entt::any begin_save_entt(entt::entity&, const conn_ptr& in_conn) {
  entity_tab::entity_tab l_entity_tab{};
  auto l_pre =
      in_conn->prepare(sqlpp::sqlite3::insert_into(l_entity_tab)
                           .set(l_entity_tab.entity_identifier = sqlpp::parameter(l_entity_tab.entity_identifier))
                           .on_conflict(l_entity_tab.entity_identifier)
                           .do_nothing());
  return std::move(l_pre);
}
void save_entt(entt::entity&, entt::entity& in_entity, entt::any& in_pre, const conn_ptr& in_conn) {
  auto& l_pre                    = entt::any_cast<entt_pie_sql_t&>(in_pre);
  l_pre.params.entity_identifier = enum_to_num(in_entity);
  (*in_conn)(l_pre);
}

entt::any begin_load_entt(entt::entity&, const conn_ptr& in_conn) {
  entity_tab::entity_tab l_entity_tab{};
  return (*in_conn)(sqlpp::select(l_entity_tab.entity_identifier).from(l_entity_tab).unconditionally());
}
void load_entt(entt::entity&, entt::any& in_pre, const conn_ptr& in_conn) {}

std::underlying_type_t<entt::entity> get_size_entt(entt::entity&, const conn_ptr& in_conn) {
  entity_tab::entity_tab l_entity_tab{};

  for (auto&& row :
       (*in_conn)(sqlpp::select(sqlpp::count(l_entity_tab.entity_identifier)).from(l_entity_tab).unconditionally())) {
    return row.count;
  }
  return 0;
}

void destory_entt(entt::entity&, const std::vector<std::int64_t>& in_vector, const conn_ptr& in_conn) {
  detail::sql_com_destroy<entity_tab::entity_tab>(in_conn, in_vector);
}

struct init_meta {
  init_meta() {
    entt::meta<entt::entity>()
        .func<&begin_save_entt>("begin_save"_hs)
        .func<&save_entt>("save"_hs)
        .func<&begin_load_entt>("begin_load"_hs)
        .func<&load_entt>("load"_hs)
        .func<&get_size_entt>("get_size"_hs)
        .func<&create_table>("create_table"_hs)
        .func<&destory_entt>("destroy"_hs);
  }
};
}  // namespace

void sqlite_snapshot::init_base_meta() { static init_meta l_init_meta{}; }
}  // namespace doodle::snapshot