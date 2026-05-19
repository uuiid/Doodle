#include <doodle_lib/sqlite_orm/orm/alias.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {
std::string alias_info_t::get_table_name(const storage& s) const {
  if (table_name_.empty()) throw std::runtime_error("Table name is required for alias");
  return fmt::format("{} AS {}", s.get_table_name(table_type_index_), table_name_);
}

std::string alias_column_info_t::get_column_name(const storage& s, bool include_table_name) const {
  auto l_column_name = s.get_column_name(ptr_, false);
  return fmt::format("{}.{}", table_alias_name_, l_column_name);
}
std::string alias_column_info_t::get_table_name(const storage& s) const {
  return fmt::format("{} AS {}", s.get_table_name(ptr_.table_type_index_), table_alias_name_);
}

void alias_column_info_t ::set_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const {
  ptr_.set_value(stmt, columnIndex, out_value);
}
void alias_column_info_t ::set_struct_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const {
  ptr_.set_struct_value(stmt, columnIndex, out_value);
}
}  // namespace doodle::orm