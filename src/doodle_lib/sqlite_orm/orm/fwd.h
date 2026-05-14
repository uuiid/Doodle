#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/exception.h>

typedef struct sqlite3_stmt sqlite3_stmt;

namespace doodle::orm {

template <typename... TableColumns>
struct select_result_type_iterator;

class storage;
struct select_t;
struct delete_t;
struct update_t;
struct insert_t;
struct create_trigger_t;

template <typename...>
inline constexpr bool always_false = false;


template <typename T>
struct column_operations;

template <typename T>
struct is_column_operations_specialization : std::false_type {};

template <typename T>
struct is_column_operations_specialization<column_operations<T>> : std::true_type {};

template <typename T>
struct sqlite_statement_binder {
  std::int32_t bind(sqlite3_stmt* stmt, int index, const T& value) const;
};
template <typename T>
struct sqlite_statement_extractor {
  T extract(sqlite3_stmt* stmt, int columnIndex) const;
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
  const column_type operator()() const;
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

template <typename Table>
struct object_t {
  using table_type = Table;
  table_type obj_;
  object_t() = default;
  explicit object_t(const table_type& obj) : obj_(obj) {}
  operator std::type_index() const { return std::type_index{typeid(Table)}; }
};

template <typename Table>
auto object() {
  return object_t<Table>{};
}
template <typename Table>
auto object(const Table& obj) {
  return object_t<Table>{obj};
}

template <typename T>
struct is_object_specialization : std::false_type {};
template <typename T>
struct is_object_specialization<object_t<T>> : std::true_type {};
template <typename T>
inline constexpr bool is_object_specialization_v = is_object_specialization<std::remove_cvref_t<T>>::value;

struct column_operations_base_t {
 protected:
  column_operations_base_t() = default;

 public:
  // to sql operator
  virtual std::string to_sql(const storage& s, bool include_table_name) const                                   = 0;
  // 创建bind参数
  // 收集bind参数
  virtual void collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const = 0;
  virtual std::string get_column_name(const storage& s) const                                                   = 0;
};

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