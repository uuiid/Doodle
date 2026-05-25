#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/exception.h>

#include <concepts>

typedef struct sqlite3_stmt sqlite3_stmt;

namespace doodle::orm {

template <typename... TableColumns>
struct select_result_type_iterator;

class storage;
struct select_t;
struct delete_t;
struct update_t;
struct insert_t;
struct sqlite_stmt;
struct create_trigger_t;
struct bind_value_collector_t;
struct on_operations;

template <typename...>
inline constexpr bool always_false = false;

template <typename T, typename Enable = void>
struct sqlite_statement_binder {
  std::int32_t bind(sqlite3_stmt* stmt, int index, const T& value) const {
    static_assert(always_false<T>, "No binder defined for this type");
    return 0;
  }
};
template <typename T, typename Enable = void>
struct sqlite_statement_extractor {
  T extract(sqlite3_stmt* stmt, int columnIndex) const {
    static_assert(always_false<T>, "No extractor defined for this type");
    return T{};
  }
};

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
// 序列化 sql 上下文标准
class to_sql_ctx {
 public:
  enum to_sql_ctx_e {  // 这些上下文会影响 column_operations 中 operator=, operator== 等操作符生成的 SQL
                       // 片段中是否是bind参数，还是直接使用值
    select_sql,
    insert_sql,
    update_sql,
    delete_sql,

    // 这些上下文会传递给 column_operations 中的 to_sql 函数，以便生成不同的 SQL 片段
    create_trigger_sql,
    create_unique_index_sql,
    create_index_sql,
    create_table_sql,
  };
  to_sql_ctx_e ctx_{select_sql};
  bool is_bind_param_{true};  // 是否生成 bind 参数，还是直接使用值
};

template <typename T, typename Enable = void>
struct sqlite_statement_printer {
  const column_type operator()() const {
    static_assert(always_false<T>, "No printer defined for this type");
    return column_type::null;
  }
};

template <typename T>
struct class_attr_type;  // 主模板：不定义，仅用于特化

// 特化：数据成员指针 (T C::*)
template <typename C, typename T>
struct class_attr_type<T C::*> {
  using ptr_type    = T;
  using class_type  = C;
  using result_type = T;
};

// 辅助别名
template <typename T>
using class_attr_type_t = typename class_attr_type<T>::ptr_type;
template <typename T>
using class_type_t = typename class_attr_type<T>::class_type;
template <typename T>
using class_result_type_t = typename class_attr_type<T>::result_type;

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

// 特化：object<Table>
template <typename Table>
struct class_attr_type<object_t<Table>> {
  using ptr_type    = void;  // object<Table> 不对应具体成员指针，因此使用 void 占位
  using class_type  = Table;
  using result_type = Table;
};

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
  virtual std::string to_sql(const storage& s, to_sql_ctx ctx) const              = 0;
  // 创建bind参数
  // 收集bind参数
  virtual void collect_bind_variants(bind_value_collector_t& bind_variants) const = 0;
  virtual std::string get_column_name(const storage& s, to_sql_ctx ctx) const     = 0;
};

// 运行是表基类, 可以获取表名称
struct table_info_base_t {
  virtual ~table_info_base_t()                               = default;
  virtual std::string get_table_name(const storage& s) const = 0;
};
using table_info_base_ptr = std::shared_ptr<table_info_base_t>;
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