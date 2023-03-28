#pragma once

#include "doodle_core/doodle_core_fwd.h"

#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
namespace doodle::database_n::detail {

template <typename T>
void sql_com_destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  auto& l_conn = *in_ptr;
  T l_tabl{};
  auto l_pre = l_conn.prepare(sqlpp::remove_from(l_tabl).where(l_tabl.entityId == sqlpp::parameter(l_tabl.entityId)));

  for (auto& l_id : in_handle) {
    l_pre.params.entityId = boost::numeric_cast<std::int64_t>(l_id);
    l_conn(l_pre);
  }
}
template <typename T>
inline sqlpp::make_traits<T, sqlpp::tag::can_be_null> can_be_null();

template <typename T>
inline sqlpp::make_traits<T, sqlpp::tag::require_insert> require_insert();

}  // namespace doodle::database_n::detail

#define DOODLE_SQL_COLUMN_IMP(column_name, type, tag)                                                 \
  struct column_name {                                                                                \
    struct _alias_t {                                                                                 \
      static constexpr const char _literal[] = #column_name;                                          \
      using _name_t                          = sqlpp::make_char_sequence<sizeof(_literal), _literal>; \
      template <typename T>                                                                           \
      struct _member_t {                                                                              \
        T column_name;                                                                                \
        T& operator()() { return column_name; }                                                       \
        const T& operator()() const { return column_name; }                                           \
      };                                                                                              \
    };                                                                                                \
    using _traits = decltype(tag<type>());                                                            \
  }

#define DOODLE_SQL_TABLE_IMP(table_name, ...)                                                         \
  struct table_name : sqlpp::table_t<table_name, __VA_ARGS__> {                                       \
    struct _alias_t {                                                                                 \
      static constexpr const char _literal[] = #table_name;                                           \
      using _name_t                          = sqlpp::make_char_sequence<sizeof(_literal), _literal>; \
      template <typename T>                                                                           \
      struct _member_t {                                                                              \
        T table_name;                                                                                 \
        T& operator()() { return table_name; }                                                        \
        const T& operator()() const { return table_name; }                                            \
      };                                                                                              \
    };                                                                                                \
  }
