#include <doodle_lib/sqlite_orm/orm/alias_subquery.h>
#include <doodle_lib/sqlite_orm/orm/session.h>

namespace doodle::orm {

// std::string subquery_alias_info_t::get_table_name(const storage& s) const {
//   to_sql_ctx l_ctx{};
//   l_ctx.ctx_ |= to_sql_ctx::select_sql;  // 强制使用 subquery_sql 上下文，以确保生成正确的 SQL 片段格式
//   auto subquery_sql = subquery_.to_sql(s, l_ctx);
//   return fmt::format("({}) AS {}", subquery_sql, alias_name_);
// }
std::string subquery_alias_info_t::to_sql(const session& s, const to_sql_ctx& ctx) const {
  to_sql_ctx l_ctx = ctx;
  l_ctx.ctx_ |= to_sql_ctx::select_sql;  // 强制使用 subquery_sql 上下文，以确保生成正确的 SQL 片段格式
  auto subquery_sql = subquery_.to_sql(s, l_ctx);
  return fmt::format("({}) AS {}", subquery_sql, alias_name_);
}
void subquery_alias_info_t::collect_bind_variants(bind_value_collector_t& bind_variants) const {
  subquery_.collect_bind_variants(bind_variants);
}

}  // namespace doodle::orm