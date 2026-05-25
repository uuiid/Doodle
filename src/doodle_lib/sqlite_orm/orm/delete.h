#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {
struct delete_t {
 private:
  friend class storage;
  friend delete_t delete_from(storage& s);

  std::string from_table_name_;
  std::shared_ptr<column_operations_base_t> wheres_;
  storage* s_{nullptr};
  std::shared_ptr<sqlite_stmt> stmt_;
  bind_value_collector_t bind_variants_{};

 public:
  template <typename T>
  delete_t& where(T&& condition_fun) {
    auto l_condition_fun_ptr = std::make_shared<T>(std::forward<T>(condition_fun));
    wheres_                  = l_condition_fun_ptr;
    return *this;
  }
  template <typename FromTable>
  delete_t& from() {
    from_table_name_ = s_->get_table_name<FromTable>();
    return *this;
  }

  delete_t& operator()();

  std::string to_sql(to_sql_ctx ctx) const;
};
inline delete_t delete_from(storage& s) {
  delete_t l_delete{};
  l_delete.s_ = &s;
  return l_delete;
}
}  // namespace doodle::orm