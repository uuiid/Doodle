#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>
#include <doodle_lib/sqlite_orm/orm/update.h>

#include <string>
#include <vector>

namespace doodle::orm {
std::string update_t::to_sql(to_sql_ctx ctx) const {
  if (!wheres_) throw std::runtime_error("WHERE condition is required for UPDATE operation");
  auto l_ctx = ctx;
  l_ctx.ctx_ |= to_sql_ctx::update_sql;  // 强制使用 update_sql 上下文，以确保生成正确的 SQL 片段格式

  std::vector<std::string> l_set_clauses;
  for (const auto& col_op : column_operations_) {
    l_set_clauses.push_back(col_op->to_sql(*s_, l_ctx));
  }
  auto l_sql = fmt::format(
      "UPDATE {} SET {} WHERE {}", from_table_name_, fmt::join(l_set_clauses, ", "), wheres_->to_sql(*s_, l_ctx)
  );
  return l_sql;
}
update_t& update_t::operator()() & {
  if (!stmt_) {
    to_sql_ctx l_ctx{.ctx_ = to_sql_ctx::update_sql};
    auto l_sql = to_sql(l_ctx);
    stmt_      = std::make_shared<sqlite_stmt>();
    stmt_->prepare(*s_, l_sql);
    bind_variants_.bind_values_.clear();
    for (const auto& col_op : column_operations_) {
      col_op->collect_bind_variants(bind_variants_);
    }
    wheres_->collect_bind_variants(bind_variants_);
  }
  stmt_->reset_bind();
  for (const auto& val : bind_variants_.bind_values_) val.bind(*stmt_);

  stmt_->step();
  return *this;
}
}  // namespace doodle::orm