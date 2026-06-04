#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/bind_value.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>

#include <any>
#include <functional>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <utility>

namespace doodle::orm {
class storage;
struct sqlite_stmt;
// 选择时传递列类型的专用模板类
template <typename Table, typename Value, typename BaseClass>
struct result_column_info_t : public BaseClass {
  using value_type       = Value;
  using table_type       = Table;
  using base_class_type  = BaseClass;

  result_column_info_t() = default;
  template <typename... Args>
    requires(std::is_constructible_v<BaseClass, Args...>)
  explicit result_column_info_t(Args&&... args) : BaseClass(std::forward<Args>(args)...) {}
};
// 是 result_column_info_t 的特化
template <typename T>
struct is_result_column_info_t : std::false_type {};
template <typename Table, typename Value, typename BaseClass>
struct is_result_column_info_t<result_column_info_t<Table, Value, BaseClass>> : std::true_type {};
template <typename T>
inline constexpr bool is_result_column_info_t_v = is_result_column_info_t<std::remove_cvref_t<T>>::value;

// 特化：class_attr_type
template <typename Table, typename Value, typename BaseClass>
  requires(!std::is_void_v<Table> && !std::is_void_v<Value>)
struct class_attr_type<result_column_info_t<Table, Value, BaseClass>> {
  using ptr_type    = Value Table::*;
  using class_type  = Table;  // result_column_info_t 不对应具体类，因此使用 void 占位
  using result_type = Value;
};

template <typename Value, typename BaseClass>
struct class_attr_type<result_column_info_t<void, Value, BaseClass>> {
  using ptr_type    = void;
  using class_type  = void;
  using result_type = Value;
};

template <typename Table, typename BaseClass>
struct class_attr_type<result_column_info_t<Table, void, BaseClass>> {
  using ptr_type    = void;
  using class_type  = Table;
  using result_type = void;
};

// 运行时列信息
struct base_column_info_t {
  virtual ~base_column_info_t()                                                                            = default;
  virtual std::string get_column_name(const storage& s, const to_sql_ctx& ctx) const                       = 0;
  virtual void set_value(const sqlite_stmt& stmt, int columnIndex, const std::any& out_value) const        = 0;
  virtual void set_struct_value(const sqlite_stmt& stmt, int columnIndex, const std::any& out_value) const = 0;
};

struct table_columns_t {
  std::type_index table_type_index_{typeid(void)};
  std::any any_value_{};
  // 这个类是可复制的, 因此需要确保 std::function 的复制行为正确, 因此, 不可以捕获 this指针
  std::function<bool(const table_columns_t&)> equals_{};
  std::function<void(const sqlite_stmt&, int, std::any)> set_value_{};
  std::function<void(const sqlite_stmt&, int, std::any)> set_struct_value_{};
  std::function<bind_value_t(const std::any&)> get_bind_value_{};

  table_columns_t() = default;

  template <typename ValueType, typename Table>
  explicit table_columns_t(ValueType Table::* in_ptr);

  template <typename ValueType, typename Table>
  bool is(ValueType Table::* in_ptr) const {
    if (any_value_.type() != typeid(ValueType Table::*)) return false;
    auto l_ptr = std::any_cast<ValueType Table::*>(any_value_);
    return l_ptr && l_ptr == in_ptr;
  }

  bool operator==(const table_columns_t& other) const { return equals_ && equals_(other); }
  bool operator!=(const table_columns_t& other) const { return !(*this == other); }
  // bool operator
  operator bool() const { return any_value_.has_value(); }

  template <typename Table>
  bind_value_t get_value(const Table& obj) const {
    if (!get_bind_value_) throw std::runtime_error("Get bind value function is not initialized");
    std::any l_any{&obj};
    return get_bind_value_(l_any);
  }
  void set_value(const sqlite_stmt& stmt, int columnIndex, std::any out_value) const {
    if (!set_value_) throw std::runtime_error("Column setter is not initialized");
    set_value_(stmt, columnIndex, out_value);
  }
  void set_struct_value(const sqlite_stmt& stmt, int columnIndex, std::any out_value) const {
    if (!set_struct_value_) throw std::runtime_error("Struct column setter is not initialized");
    set_struct_value_(stmt, columnIndex, out_value);
  }
};

// 基本的列信息，包含列名和成员指针

struct column_info_t : public base_column_info_t {
  table_columns_t ptr_;
  template <typename ValueType, typename Table>
  explicit column_info_t(ValueType Table::* in_ptr) : ptr_(in_ptr) {}
  explicit column_info_t(const table_columns_t& in_column) : ptr_(in_column) {}
  std::string get_column_name(const storage& s, const to_sql_ctx& ctx) const override;
  void set_value(const sqlite_stmt& stmt, int columnIndex, const std::any& out_value) const override;
  void set_struct_value(const sqlite_stmt& stmt, int columnIndex, const std::any& out_value) const override;
};
using column_info_ptr = std::shared_ptr<base_column_info_t>;

}  // namespace doodle::orm