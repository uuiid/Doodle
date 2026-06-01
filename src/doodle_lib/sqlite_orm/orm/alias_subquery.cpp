#include <doodle_lib/sqlite_orm/orm/alias_subquery.h>

namespace doodle::orm {

std::string subquery_alias_info_t::get_table_name(const storage& s) const {
  to_sql_ctx l_ctx{};
  l_ctx.ctx_ |= to_sql_ctx::select_sql;  // 强制使用 subquery_sql 上下文，以确保生成正确的 SQL 片段格式
  auto subquery_sql = subquery_.to_sql(l_ctx);
  return fmt::format("({}) AS {}", subquery_sql, alias_name_);
}

}  // namespace doodle::orm