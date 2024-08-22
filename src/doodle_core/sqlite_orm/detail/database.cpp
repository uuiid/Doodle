//
// Created by TD on 2024/3/13.
//
#include "all.h"
//
#include <doodle_core/core/core_sql.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/sqlite3/sqlite3.h>
#include <doodle_core/sqlite_orm/sqlite_base.h>
//
#include <doodle_core/metadata/metadata.h>
//
#include <boost/preprocessor.hpp>

#include <entt/entt.hpp>
#include <sqlpp11/ppgen.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
// #define DOODLE_SQL_TABLE_IMP_2(r, data, elem) DOODLE_SQL_COLUMN_IMP elem

// #define DOODLE_SQL_TABLE(table_name, ...) BOOST_PP_SEQ_FOR_EACH(DOODLE_SQL_TABLE_IMP_2, _, __VA_ARGS__)
namespace doodle::snapshot {

// clang-format off
SQLPP_DECLARE_TABLE(
    (database_tab),
    (id,                 int, SQLPP_PRIMARY_KEY)
    (entity_identifier,  int, SQLPP_NOT_NULL)
    (uuid_data,          text, SQLPP_NOT_NULL)
)
// clang-format on

// DOODLE_SQL_TABLE(database_tab2, (id, sqlpp::integer, detail::can_be_null)(id2, sqlpp::integer, detail::can_be_null))

namespace {

void create_table(const sql_connection_ptr& in_conn) {
  detail::sql_table_base<database_tab::database_tab> l_tab{};
  l_tab.create_table(in_conn);
}
std::shared_ptr<void> begin_save(const sql_connection_ptr& in_conn) {
  database_tab::database_tab l_entity_tab{};
  auto l_pre =
      in_conn->prepare(sqlpp::sqlite3::insert_into(l_entity_tab)
                           .set(
                               l_entity_tab.entity_identifier = sqlpp::parameter(l_entity_tab.entity_identifier),
                               l_entity_tab.uuid_data         = sqlpp::parameter(l_entity_tab.uuid_data)
                           )
                           .on_conflict(l_entity_tab.entity_identifier)
                           .do_update(l_entity_tab.uuid_data = sqlpp::sqlite3::excluded(l_entity_tab.uuid_data)));
  return std::make_shared<decltype(l_pre)>(std::move(l_pre));
}
void save(
    const database& in_com, entt::entity& in_entity, std::shared_ptr<void>& in_pre, const sql_connection_ptr& in_conn
) {
  database_tab::database_tab l_entity_tab{};
  using pre_type                  = decltype(in_conn->prepare(
      sqlpp::sqlite3::insert_into(l_entity_tab)
          .set(
              l_entity_tab.entity_identifier = sqlpp::parameter(l_entity_tab.entity_identifier),
              l_entity_tab.uuid_data         = sqlpp::parameter(l_entity_tab.uuid_data)
          )
          .on_conflict(l_entity_tab.entity_identifier)
          .do_update(l_entity_tab.uuid_data = sqlpp::sqlite3::excluded(l_entity_tab.uuid_data))
  ));
  auto l_pre                      = std::static_pointer_cast<pre_type>(in_pre);
  l_pre->params.entity_identifier = enum_to_num(in_entity);
  l_pre->params.uuid_data         = boost::uuids::to_string(in_com.uuid());
  (*in_conn)(*l_pre);
}
void destroy(const std::vector<std::int64_t>& in_vector, const sql_connection_ptr& in_conn) {
  detail::sql_com_destroy<database_tab::database_tab>(in_conn, in_vector);
}

std::underlying_type_t<entt::entity> get_size(const sql_connection_ptr& in_conn) {
  database_tab::database_tab l_tabl{};
  for (auto&& raw : (*in_conn)(sqlpp::select(sqlpp::count(l_tabl.id)).from(l_tabl).unconditionally())) {
    return raw.count.value();
  }
  return 0;
}
using pre_rus_t = decltype(std::declval<sqlpp::sqlite3::connection>()(
    sqlpp::select(
        std::declval<database_tab::database_tab>().entity_identifier,
        std::declval<database_tab::database_tab>().uuid_data
    )
        .from(std::declval<database_tab::database_tab>())
        .unconditionally()
));
std::shared_ptr<void> begin_load(const sql_connection_ptr& in_conn) {
  database_tab::database_tab l_tabl{};
  auto l_r = (*in_conn)(sqlpp::select(l_tabl.entity_identifier, l_tabl.uuid_data).from(l_tabl).unconditionally());
  return std::make_shared<decltype(l_r)>(std::move(l_r));
}
void load_entt(entt::entity& in_entity, std::shared_ptr<void>& in_pre) {
  auto l_pre = std::static_pointer_cast<pre_rus_t>(in_pre);
  in_entity  = num_to_enum<entt::entity>(l_pre->front().entity_identifier.value());
}
bool has_table(const sql_connection_ptr& in_conn) {
  database_tab::database_tab l_tab{};
  detail::sql_table_base<database_tab::database_tab> l_table{};
  return l_table.has_table(in_conn);
}

}  // namespace
void load_com(database& in_entity, std::shared_ptr<void>& in_pre) {
  auto l_pre        = std::static_pointer_cast<pre_rus_t>(in_pre);
  in_entity.p_uuid_ = boost::lexical_cast<boost::uuids::uuid>(l_pre->front().uuid_data.value());
  l_pre->pop_front();
}

void reg_database() {
  entt::meta<database>()
      .func<&create_table>("create_table"_hs)
      .func<&begin_save>("begin_save"_hs)
      .func<&save>("save"_hs)
      .func<&destroy>("destroy"_hs)
      .func<&get_size>("get_size"_hs)
      .func<&begin_load>("begin_load"_hs)
      .func<&load_entt>("load_entt"_hs)
      .func<&load_com>("load_com"_hs)
      .func<&has_table>("has_table"_hs);
}

}  // namespace doodle::snapshot