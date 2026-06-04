#include "count.h"

#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {
std::string count_column_info_t::get_column_name(const storage& s, const to_sql_ctx& ctx) const {
  if (!column_infos_) return "COUNT(*)";

  return fmt::format("COUNT({})", s.get_column_name(column_infos_, ctx));
}

void count_column_info_t::set_value(const sqlite_stmt& stmt, int columnIndex, const std::any& out_value) const {
  if (!out_value.has_value()) throw std::runtime_error("Output value pointer is null");
  if (out_value.type() != typeid(std::int64_t*)) throw std::runtime_error("Output value type mismatch");
  auto l_any                          = out_value;
  std::int64_t count_result           = stmt.get_column_value<std::int64_t>(columnIndex);
  *std::any_cast<std::int64_t*>(l_any) = count_result;
}
void count_column_info_t::set_struct_value(const sqlite_stmt& stmt, int columnIndex, const std::any& out_value) const {
  // COUNT 列不支持结构体绑定，因此直接抛出异常
  throw std::runtime_error("COUNT column does not support struct value binding");
}
}  // namespace doodle::orm