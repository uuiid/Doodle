#include "count.h"

#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {
std::string count_column_info_t::get_column_name(const storage& s, bool include_table_name) const {
  if (!column_infos_) return "COUNT(*)";

  return fmt::format("COUNT({})", s.get_column_name(column_infos_, include_table_name));
}

std::string count_column_info_t::get_table_name(const storage& /*s*/) const {
  return "";  // COUNT 列没有具体的表名
}

void count_column_info_t::set_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const {
  if (!out_value) throw std::runtime_error("Output value pointer is null");
  std::int64_t count_result              = stmt.get_column_value<std::int64_t>(columnIndex);
  *static_cast<std::int64_t*>(out_value) = count_result;
}
void count_column_info_t::set_struct_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const {
  // COUNT 列不支持结构体绑定，因此直接抛出异常
  throw std::runtime_error("COUNT column does not support struct value binding");
}
}  // namespace doodle::orm