#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/exception.h>
typedef struct sqlite3_stmt sqlite3_stmt;

namespace doodle::orm {
using storage_column_types =
    std::tuple<std::int64_t, std::double_t, std::string, uuid, chrono::system_zoned_time, nlohmann::json, FSys::path>;
template <typename... TableColumns>
struct select_result_type_iterator;

class storage;
struct select_t;

template <typename...>
inline constexpr bool always_false = false;

template <typename Table, typename Tuple>
struct tuple_to_table_member_variant;
template <typename Table, typename... Ts>
struct tuple_to_table_member_variant<Table, std::tuple<Ts...>> {
  using type = std::variant<Ts Table::*...>;
};

template <typename Table>
using table_columns_t = typename tuple_to_table_member_variant<Table, storage_column_types>::type;

template <typename T>
struct column_operations;

template <typename T>
struct is_column_operations_specialization : std::false_type {};

template <typename T>
struct is_column_operations_specialization<column_operations<T>> : std::true_type {};

template <typename T>
struct sqlite_statement_binder {
  std::int32_t bind(sqlite3_stmt* stmt, int index, const T& value) {
    static_assert(always_false<T>, "没有为该类型定义绑定器");
    return -1;
  }
};
template <typename T>
struct sqlite_statement_extractor {
  T extract(sqlite3_stmt* stmt, int columnIndex) {
    static_assert(always_false<T>, "没有为该类型定义提取器");
    return T{};
  }
};

template <typename T>
inline constexpr bool is_column_operations_specialization_v =
    is_column_operations_specialization<std::remove_cvref_t<T>>::value;

#define DOODLE_ORM_ERROR_SQLITE3(error_code, sqlite3_ptr)                                         \
  switch (error_code) {                                                                           \
    case SQLITE_OK:                                                                               \
    case SQLITE_ROW:                                                                              \
    case SQLITE_DONE:                                                                             \
      break;                                                                                      \
    default:                                                                                      \
      throw sqlite_orm_exception(fmt::format("{}: {}", error_code, sqlite3_errmsg(sqlite3_ptr))); \
  }

template <typename T>
struct name_and_type_ptr {
  std::string name_;
  table_columns_t<T> ptr_;

  using column_type = table_columns_t<T>;
};
enum class column_type {
  null,
  integer,
  real,
  text,
  blob,
};

enum class foreign_key_action {
  no_action,
  restrict,
  set_null,
  set_default,
  cascade,
};

enum class where_op {
  equal,
  not_equal,
  greater,
  less,
  greater_equal,
  less_equal,
  like,
  in,
};

enum class join_type {
  inner,
  left,
  right,
  full,
};

template <typename T>
struct sqlite_statement_printer {
  const column_type operator()() const {
    static_assert(always_false<T>, "没有为该类型定义打印器");
    static column_type type = column_type::null;
    return type;
  }
};

template <typename T, typename Tuple>
struct is_in_tuple : std::false_type {};

template <typename T, typename... Ts>
struct is_in_tuple<T, std::tuple<Ts...>> : std::disjunction<std::is_same<T, Ts>...> {};

template <typename T, typename Tuple>
inline constexpr bool is_in_tuple_v = is_in_tuple<T, Tuple>::value;

template <typename T>
struct member_type;  // 主模板：不定义，仅用于特化

// 特化：数据成员指针 (T C::*)
template <typename C, typename T>
struct member_type<T C::*> {
  using ptr_type   = T;
  using class_type = C;
};
// 辅助别名
template <typename T>
using member_type_t = typename member_type<T>::ptr_type;
template <typename T>
using member_class_type_t = typename member_type<T>::class_type;






}  // namespace doodle::orm

namespace fmt {
// join_type 格式化
template <>
struct formatter<doodle::orm::join_type> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(doodle::orm::join_type joinType, FormatContext& ctx) const -> decltype(ctx.out()) {
    std::string_view name;
    switch (joinType) {
      case doodle::orm::join_type::inner:
        name = "INNER JOIN";
        break;
      case doodle::orm::join_type::left:
        name = "LEFT JOIN";
        break;
      case doodle::orm::join_type::right:
        name = "RIGHT JOIN";
        break;
      case doodle::orm::join_type::full:
        name = "FULL JOIN";
        break;
    }
    format_to(ctx.out(), "{}", name);
    return ctx.out();
  }
};

// clumn_type 格式化
template <>
struct formatter<doodle::orm::column_type> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(doodle::orm::column_type columnType, FormatContext& ctx) const -> decltype(ctx.out()) {
    std::string_view name;
    switch (columnType) {
      case doodle::orm::column_type::null:
        name = "NULL";
        break;
      case doodle::orm::column_type::integer:
        name = "INTEGER";
        break;
      case doodle::orm::column_type::real:
        name = "REAL";
        break;
      case doodle::orm::column_type::text:
        name = "TEXT";
        break;
      case doodle::orm::column_type::blob:
        name = "BLOB";
        break;
    }
    format_to(ctx.out(), "{}", name);
    return ctx.out();
  }
};
// foreign_key_action 格式化
template <>
struct formatter<doodle::orm::foreign_key_action> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(doodle::orm::foreign_key_action action, FormatContext& ctx) const -> decltype(ctx.out()) {
    std::string_view name;
    switch (action) {
      case doodle::orm::foreign_key_action::no_action:
        name = "NO ACTION";
        break;
      case doodle::orm::foreign_key_action::restrict:
        name = "RESTRICT";
        break;
      case doodle::orm::foreign_key_action::set_null:
        name = "SET NULL";
        break;
      case doodle::orm::foreign_key_action::set_default:
        name = "SET DEFAULT";
        break;
      case doodle::orm::foreign_key_action::cascade:
        name = "CASCADE";
        break;
    }
    format_to(ctx.out(), "{}", name);
    return ctx.out();
  }
};
}  // namespace fmt