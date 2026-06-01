#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/delete.h>

namespace doodle::orm {

void delete_t::prepare(storage& s, const to_sql_ctx& ctx) {
  auto l_sql    = to_sql(s, ctx);
  state_->stmt_ = std::make_shared<sqlite_stmt>();
  state_->stmt_->prepare(s, l_sql);
  state_->bind_variants_.bind_values_.clear();
  collect_bind_variants(state_->bind_variants_);
}

void delete_t::collect_bind_variants(bind_value_collector_t& bind_variants) const {
  if (state_->wheres_) state_->wheres_->collect_bind_variants(bind_variants);
}

std::string delete_t::to_sql(const storage& s, const to_sql_ctx& ctx) const {
  if (!state_->wheres_) throw std::runtime_error("WHERE condition is required for DELETE operation");
  auto l_ctx = ctx;
  l_ctx.ctx_ |= to_sql_ctx::delete_sql;  // 强制使用 delete_sql 上下文，以确保生成正确的 SQL 片段格式
  auto l_sql = fmt::format("DELETE FROM {} WHERE {}", state_->from_table_name_, state_->wheres_->to_sql(s, l_ctx));
  return l_sql;
}

delete_t delete_t::operator()() {
  if (!state_->stmt_) prepare(*state_->s_, to_sql_ctx{.ctx_ = to_sql_ctx::delete_sql});

  state_->stmt_->reset_bind();
  for (const auto& val : state_->bind_variants_.bind_values_) val.bind(*state_->stmt_);
  state_->stmt_->step();
  return *this;
}
}  // namespace doodle::orm