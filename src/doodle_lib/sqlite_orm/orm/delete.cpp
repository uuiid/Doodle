#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/delete.h>

namespace doodle::orm {
std::string delete_t::to_sql(to_sql_ctx ctx) const {
  if (!wheres_) throw std::runtime_error("WHERE condition is required for DELETE operation");
  auto l_ctx = ctx;
  l_ctx.ctx_ |= to_sql_ctx::delete_sql;  // 强制使用 delete_sql 上下文，以确保生成正确的 SQL 片段格式
  auto l_sql = fmt::format("DELETE FROM {} WHERE {}", from_table_name_, wheres_->to_sql(*s_, l_ctx));
  return l_sql;
}
delete_t& delete_t::operator()() {
  if (!stmt_) {
    to_sql_ctx l_ctx{.ctx_ = to_sql_ctx::delete_sql};
    auto l_sql = to_sql(l_ctx);
    stmt_      = std::make_shared<sqlite_stmt>();
    stmt_->prepare(*s_, l_sql);
    wheres_->collect_bind_variants(bind_variants_);
  }
  stmt_->reset_bind();
  for (const auto& val : bind_variants_.bind_values_) val.bind(*stmt_);
  stmt_->step();
  return *this;
}
}  // namespace doodle::orm