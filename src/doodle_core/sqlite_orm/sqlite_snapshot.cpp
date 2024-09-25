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

void create_table(const sql_connection_ptr& in_conn) {
  detail::sql_table_base<entity_tab::entity_tab> l_tab{};
  l_tab.create_table(in_conn);
}

std::shared_ptr<void> begin_save_entt(const sql_connection_ptr& in_conn) {
  entity_tab::entity_tab l_entity_tab{};
  auto l_pre =
      in_conn->prepare(sqlpp::sqlite3::insert_into(l_entity_tab)
                           .set(l_entity_tab.entity_identifier = sqlpp::parameter(l_entity_tab.entity_identifier))
                           .on_conflict(l_entity_tab.entity_identifier)
                           .do_nothing());
  return std::make_shared<decltype(l_pre)>(std::move(l_pre));
}
void save_entt(
    entt::entity&, entt::entity& in_entity, std::shared_ptr<void>& in_pre, const sql_connection_ptr& in_conn
) {
  auto l_pre                      = std::static_pointer_cast<entt_pie_sql_t>(in_pre);
  l_pre->params.entity_identifier = enum_to_num(in_entity);
  (*in_conn)(*l_pre);
}

std::shared_ptr<void> begin_load_entt(const sql_connection_ptr& in_conn) {
  entity_tab::entity_tab l_entity_tab{};
  auto l_r = (*in_conn)(sqlpp::select(l_entity_tab.entity_identifier).from(l_entity_tab).unconditionally());
  return std::make_shared<decltype(l_r)>(std::move(l_r));
}
void load_entt(entt::entity&, entt::entity& in_entity, std::shared_ptr<void>& in_pre) {
  using pre_rus_t = decltype(std::declval<sqlpp::sqlite3::connection>()(
      sqlpp::select(entity_tab::entity_tab{}.entity_identifier).from(entity_tab::entity_tab{}).unconditionally()
  ));
  auto l_pre      = std::static_pointer_cast<pre_rus_t>(in_pre);
  in_entity       = num_to_enum<entt::entity>(l_pre->front().entity_identifier.value());
  l_pre->pop_front();
}

std::underlying_type_t<entt::entity> get_size_entt(const sql_connection_ptr& in_conn) {
  entity_tab::entity_tab l_entity_tab{};

  for (auto&& row :
       (*in_conn)(sqlpp::select(sqlpp::count(l_entity_tab.entity_identifier)).from(l_entity_tab).unconditionally())) {
    return row.count.value();
  }
  return 0;
}

void destory_entt(const std::vector<std::int64_t>& in_vector, const sql_connection_ptr& in_conn) {
  detail::sql_com_destroy<entity_tab::entity_tab>(in_conn, in_vector);
}

bool has_entt_table(const sql_connection_ptr& in_conn) {
  entity_tab::entity_tab l_entity_tab{};
  detail::sql_table_base<entity_tab::entity_tab> l_tab{};
  return l_tab.has_table(in_conn);
}

void reg_entt() {
  entt::meta<entt::entity>()
      .func<&begin_save_entt>("begin_save"_hs)
      .func<&save_entt>("save"_hs)
      .func<&begin_load_entt>("begin_load"_hs)
      .func<&load_entt>("load_entt"_hs)
      .func<&get_size_entt>("get_size"_hs)
      .func<&create_table>("create_table"_hs)
      .func<&has_entt_table>("has_table"_hs)
      .func<&destory_entt>("destroy"_hs);
}

struct init_meta {
  init_meta() { reg_entt(); }
};
}  // namespace

void sqlite_snapshot::init_base_meta() { static init_meta l_init_meta{}; }
}  // namespace doodle::snapshot