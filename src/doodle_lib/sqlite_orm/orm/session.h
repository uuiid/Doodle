#pragma once

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {
class session {
  sqlite_connection_guard_t connection_guard_;
  storage* s_{nullptr};

 public:
  explicit session(storage& s);
  ~session();

  template <typename T>
  bool has_reg_table() {
    return s_->has_reg_table<T>();
  }

  template <typename T>
  std::string get_column_name(auto T::* in_ptr, const to_sql_ctx& ctx) const {
    return s_->get_column_name(in_ptr, ctx);
  }
  std::string get_column_name(const table_columns_t& in_column, const to_sql_ctx& ctx) const {
    return s_->get_column_name(in_column, ctx);
  }

  template <typename T>
  const std::vector<column_info>& get_table_columns() const {
    return s_->get_table_columns<T>();
  }
  template <typename T>
  std::string get_table_name() const {
    return s_->get_table_name<T>();
  }
  std::string get_table_name(std::type_index in_type_index) const { return s_->get_table_name(in_type_index); }
};
}  // namespace doodle::orm