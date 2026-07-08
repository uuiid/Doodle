#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {
std::string column_info_t::get_column_name(const session& s, const to_sql_ctx& ctx) const {
  return s.get_column_name(ptr_, ctx);
}

// void column_info_t::set_value(const sqlite_stmt& stmt, int columnIndex, const std::any& out_value) const {
//   ptr_.set_value(stmt, columnIndex, out_value);
// }
void column_info_t::set_struct_value(const sqlite_stmt& stmt, int columnIndex, const std::any& out_value) const {
  ptr_.set_struct_value(stmt, columnIndex, out_value);
}
}  // namespace doodle::orm