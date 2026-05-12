#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <vector>

namespace doodle::orm {
struct insert_t {
 private:
  friend class storage;

  template <typename Table>
  friend auto insert() -> insert_t;

  std::vector<std::function<std::string(const storage&)>> column_names_fun_;
  std::function<void(sqlite_stmt&)> bind_fun_;
  std::type_index into_table_type_index_{typeid(void)};

 public:
  template <typename T>
    requires(is_column_operations_specialization_v<T>)
  void set(T&& in_column) {
    column_names_fun_.push_back([in_column_fun](const storage& s) { return s.get_column_name(in_column_fun); });
 
  }
};

template <typename Table>
auto insert() -> insert_t {
  insert_t l_ret{};
  l_ret.into_table_type_index_ = std::type_index{typeid(Table)};
  return l_ret;
}
}  // namespace doodle::orm