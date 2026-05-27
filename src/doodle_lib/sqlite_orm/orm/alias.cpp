#include <doodle_lib/sqlite_orm/orm/alias.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>
#include <doodle_lib/sqlite_orm/orm/table_info.h>

namespace doodle::orm {
std::string alias_info_t::get_table_name(const storage& s) const {
  if (table_name_.empty()) throw std::runtime_error("Table name is required for alias");
  return fmt::format("{} AS {}", s.get_table_name(table_type_index_), table_name_);
}

std::string alias_column_info_t::get_column_name(const storage& s, const to_sql_ctx& ctx) const {
  auto l_ctx = ctx;
  l_ctx.ctx_ |= to_sql_ctx::alias_sql;  // 强制使用 alias_sql 上下文，以确保生成正确的列名格式
  auto l_column_name = s.get_column_name(ptr_, l_ctx);
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

std::string rank_info_t::get_column_name(const storage& s, const to_sql_ctx& ctx) const { return "rank"; }
std::string rank_info_t::get_table_name(const storage& s) const { return ""; }
void rank_info_t::set_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const {}
void rank_info_t::set_struct_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const {}

any_column_info_t::any_column_info_t(std::type_index in_table_index)
    : table_info_ptr_(std::make_shared<table_info_t>(in_table_index)) {}

std::string any_column_info_t::get_column_name(const storage& s, const to_sql_ctx& ctx) const {
  auto l_table_name = table_info_ptr_->get_table_name(s);
  return l_table_name;
}
std::string any_column_info_t::get_table_name(const storage& s) const { return table_info_ptr_->get_table_name(s); }
void any_column_info_t::set_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const {}
void any_column_info_t::set_struct_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const {}

}  // namespace doodle::orm