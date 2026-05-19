#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/bind_value.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>

#include <any>
#include <functional>
#include <memory>
#include <typeindex>


namespace doodle::orm {
class storage;
struct sqlite_stmt;
// 运行时列信息
struct base_column_info_t {
  virtual ~base_column_info_t()                                                                  = default;
  virtual std::string get_column_name(const storage& s, bool include_table_name) const           = 0;
  virtual std::string get_table_name(const storage& s) const                                     = 0;
  virtual void set_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const        = 0;
  virtual void set_struct_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const = 0;
};

struct table_columns_t {
  std::type_index table_type_index_{typeid(void)};
  std::any any_value_{};
  // 这个类是可复制的, 因此需要确保 std::function 的复制行为正确, 因此, 不可以捕获 this指针
  std::function<bool(const table_columns_t&)> equals_{};
  std::function<void(const sqlite_stmt&, int, void*)> set_value_{};
  std::function<void(const sqlite_stmt&, int, void*)> set_struct_value_{};
  std::function<bind_value_t(const void*)> get_bind_value_{};

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
  template <typename Table>
  bind_value_t get_value(const Table& obj) const {
    if (!get_bind_value_) throw std::runtime_error("Get bind value function is not initialized");
    return get_bind_value_(&obj);
  }
  void set_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const {
    if (!set_value_) throw std::runtime_error("Column setter is not initialized");
    set_value_(stmt, columnIndex, out_value);
  }
  void set_struct_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const {
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
  std::string get_column_name(const storage& s, bool include_table_name) const override;
  std::string get_table_name(const storage& s) const override;
  void set_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const override;
  void set_struct_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const override;
};
using column_info_ptr = std::shared_ptr<base_column_info_t>;

}  // namespace doodle::orm