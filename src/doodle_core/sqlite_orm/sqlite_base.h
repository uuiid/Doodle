//
// Created by TD on 2024/3/11.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/sqlppWarp.h>

#include <sqlpp11/ppgen.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
namespace doodle::snapshot::detail {
template <typename... t>
struct wrong : std::false_type {};

template <typename... t>
static constexpr auto wrong_v = detail::wrong<t...>::value_type();

template <typename t>
void sql_com_destroy(const sql_connection_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  auto& l_conn = *in_ptr;
  t l_tabl{};
  auto l_pre = l_conn.prepare(
      sqlpp::remove_from(l_tabl).where(l_tabl.entity_identifier == sqlpp::parameter(l_tabl.entity_identifier))
  );

  for (const auto& l_id : in_handle) {
    l_pre.params.entity_identifier = boost::numeric_cast<std::int64_t>(l_id);
    l_conn(l_pre);
  }
}

template <typename t>
void sql_com_destroy_parent_id(
    const sql_connection_ptr& in_ptr, const std::map<entt::handle, std::int64_t>& in_handle
) {
  auto& l_conn = *in_ptr;
  const t l_tabl{};
  auto l_pre = l_conn.prepare(sqlpp::remove_from(l_tabl).where(l_tabl.parent_id == sqlpp::parameter(l_tabl.parent_id)));

  for (auto&& [l_k, l_id] : in_handle) {
    l_pre.params.parent_id = boost::numeric_cast<std::int64_t>(l_id);
    l_conn(l_pre);
  }
}
template <typename t>
void sql_com_destroy_parent_id(const sql_connection_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  auto& l_conn = *in_ptr;
  const t l_tabl{};
  auto l_pre = l_conn.prepare(sqlpp::remove_from(l_tabl).where(l_tabl.parent_id == sqlpp::parameter(l_tabl.parent_id)));

  for (auto&& l_id : in_handle) {
    l_pre.params.parent_id = boost::numeric_cast<std::int64_t>(l_id);
    l_conn(l_pre);
  }
}

struct create_table_ctx {};

template <typename table_t>
struct create_table_t {
  std::string sql_data{};

 private:
  template <typename column_t, typename>
  constexpr static std::string_view get_type_name(const column_t& /*in_column*/) {
    static_assert(wrong_v<column_t>, "missing type name for column");
    return "";
  }
  template <typename column_t, std::enable_if_t<sqlpp::is_integral_t<column_t>::value>* = nullptr>
  constexpr static std::string_view get_type_name(const column_t& /*in_column*/) {
    return "INTEGER";
  }
  template <typename column_t, std::enable_if_t<sqlpp::is_text_t<column_t>::value>* = nullptr>
  constexpr static std::string_view get_type_name(const column_t& /*in_column*/) {
    return "TEXT";
  }
  template <typename column_t, std::enable_if_t<sqlpp::is_time_point_t<column_t>::value>* = nullptr>
  constexpr static std::string_view get_type_name(const column_t& /*in_column*/) {
    return "DATETIME";
  }
  template <typename column_t, std::enable_if_t<sqlpp::is_boolean_t<column_t>::value>* = nullptr>
  constexpr static std::string_view get_type_name(const column_t& /*in_column*/) {
    return "BOOLEAN";
  }

  template <typename column_t>
  auto impl_primary_key_column(const column_t& in_column) {
    sql_data += fmt::format(
        "{} {} PRIMARY KEY", sqlpp::name_of<column_t>::template char_ptr<create_table_ctx>(), get_type_name(in_column)
    );
    return *this;
  }
  template <typename column_t>
  auto impl_column(const column_t& in_column) {
    sql_data += fmt::format(
        ", {} {}", sqlpp::name_of<column_t>::template char_ptr<create_table_ctx>(), get_type_name(in_column)
    );
    return *this;
  }

 public:
  explicit create_table_t() {
    sql_data =
        fmt::format("CREATE TABLE IF NOT EXISTS {} ( ", sqlpp::name_of<table_t>::template char_ptr<create_table_ctx>());
    table_t l_table{};
    std::apply([this](auto&&... x) { column(x...); }, sqlpp::all_of(l_table));
  }

  template <typename primary_column_t, typename... columns_t>
  auto column(const primary_column_t& in_primary_column, const columns_t&... in_columns) {
    impl_primary_key_column<primary_column_t>(in_primary_column);
    (impl_column<columns_t>(in_columns), ...);
    return *this;
  }
  template <typename self_column_t>
  auto unique_column(const self_column_t& in_self_column) {
    sql_data += fmt::format(R"(, UNIQUE ({}))", sqlpp::name_of<self_column_t>::template char_ptr<create_table_ctx>());
    return *this;
  }
  template <typename self_column_t, typename foreign_column_t>
  auto foreign_column(const self_column_t& in_self_column, const foreign_column_t& in_foreign_column) {
    sql_data += fmt::format(
        R"(, FOREIGN KEY ({}) REFERENCES {} ({})  ON DELETE CASCADE ON UPDATE CASCADE)",
        sqlpp::name_of<self_column_t>::template char_ptr<create_table_ctx>(),
        sqlpp::name_of<decltype(in_foreign_column.table())>::template char_ptr<create_table_ctx>(),
        sqlpp::name_of<foreign_column_t>::template char_ptr<create_table_ctx>()
    );
    return *this;
  }

  std::string end() const { return sql_data + ");"; }
  operator std::string() const { return sql_data + ");"; }
};
template <typename column_t>
std::string create_index(const column_t& in_column) {
  return fmt::format(
      "CREATE INDEX IF NOT EXISTS {0}_ix_{1} ON {0} ({1})",
      sqlpp::name_of<decltype(in_column.table())>::template char_ptr<create_table_ctx>(),
      sqlpp::name_of<column_t>::template char_ptr<create_table_ctx>()
  );
}
template <typename column_t>
std::string has_colume(const column_t& in_column) {
  return fmt::format(
      R"(SELECT COUNT(*) AS CNTREC FROM pragma_table_info('{}') WHERE name='{}';)",
      sqlpp::name_of<decltype(in_column.table())>::template char_ptr<create_table_ctx>(),
      sqlpp::name_of<column_t>::template char_ptr<create_table_ctx>()
  );
}
template <typename column_t>
std::string create_colume(const column_t& in_column) {
  return fmt::format(
      R"(ALTER TABLE {} ADD {};)", sqlpp::name_of<decltype(in_column.table())>::template char_ptr<create_table_ctx>(),
      sqlpp::name_of<column_t>::template char_ptr<create_table_ctx>()
  );
}

template <typename table_t>
create_table_t<table_t> create_table(const table_t& /*in_table*/) {
  return create_table_t<table_t>{};
}

// clang-format off
SQLPP_DECLARE_TABLE(
  (sqlite_master),
  (type     , text )
  (name     , text )
  (tbl_name , text )
  (rootpage , int  )
  (sql      , text )
)

SQLPP_DECLARE_TABLE(
  (pragma_table),
  (name     , text )
)

// clang-format on

template <typename table_t>
struct sql_table_base {
 private:
  template <typename table_t>
  bool has_table(const table_t& /*table*/, const sql_connection_ptr& in_connection) {
    const sqlite_master::sqlite_master l_table{};
    for (auto&& row :
         (*in_connection)(sqlpp::select(sqlpp::count(l_table.name))
                              .from(l_table)
                              .where(
                                  l_table.name == (sqlpp::name_of<table_t>::template char_ptr<create_table_ctx>()) &&
                                  l_table.type == "table"
                              ))) {
      return row.count.value() > 0;
    }
    return false;
  }
  template <typename column_t>
  bool has_colume(const sql_connection_ptr& in_connection, const column_t& in_column) {
    pragma_table::pragma_table l_tab{};
    for (auto&& row : (*in_connection)(
             sqlpp::custom_query(
                 sqlpp::select(sqlpp::count(l_tab.name)),
                 sqlpp::verbatim(fmt::format(
                     "FROM pragma_table_info('{}') AS pragma_table",
                     sqlpp::name_of<decltype(in_column.table())>::template char_ptr<create_table_ctx>()
                 )),
                 sqlpp::where(
                     l_tab.name == std::string{sqlpp::name_of<column_t>::template char_ptr<create_table_ctx>()}
                 )
             )
                 .with_result_type_of(sqlpp::select(sqlpp::count(l_tab.name)))
         )) {
      return row.count.value() > 0;
    }
    return false;
  }

 private:
  template <typename table_sub_t>
  void impl_create_table_parent_id(const sql_connection_ptr& in_ptr) {
    const table_sub_t l_table{};
    in_ptr->execute(detail::create_table(l_table).foreign_column(l_table.parent_id, table_t{}.id));
    in_ptr->execute(detail::create_index(l_table.entity_identifier));
    in_ptr->execute(detail::create_index(l_table.id));
  };

 protected:
  template <typename... table_subs_t>
  void create_table_parent_id(const sql_connection_ptr& in_ptr) {
    (impl_create_table_parent_id<table_subs_t>(in_ptr), ...);
  }
  template <typename column_t>
  void create_table(const sql_connection_ptr& in_ptr, const column_t& in_column) {
    if (!has_colume(*in_ptr, in_column)) {
      in_ptr->execute(detail::create_colume(in_column));
    }
  }

 public:
  sql_table_base() = default;
  virtual void create_table(const sql_connection_ptr& in_ptr) {
    const table_t l_tables{};
    if (has_table(l_tables, in_ptr)) return;
    in_ptr->execute(detail::create_table(l_tables).unique_column(l_tables.entity_identifier));
    in_ptr->execute(detail::create_index(l_tables.id));
    in_ptr->execute(detail::create_index(l_tables.entity_identifier));
  };
  bool has_table(const sql_connection_ptr& in_ptr) {
    const table_t l_tables{};
    return has_table(l_tables, in_ptr);
  }
};
template <typename t>
inline sqlpp::make_traits<t, sqlpp::tag::can_be_null> can_be_null();

template <typename t>
inline sqlpp::make_traits<t, sqlpp::tag::require_insert> require_insert();
}  // namespace doodle::snapshot::detail

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
  };
