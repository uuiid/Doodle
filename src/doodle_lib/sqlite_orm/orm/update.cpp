#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>
#include <doodle_lib/sqlite_orm/orm/update.h>

#include <string>
#include <vector>

namespace doodle::orm {

void update_t::prepare(session& s, const to_sql_ctx& ctx) {
  auto l_sql    = to_sql(s, ctx);
  state_->stmt_ = std::make_shared<sqlite_stmt>();
  state_->stmt_->prepare(s, l_sql);
  state_->bind_variants_.bind_values_.clear();
  collect_bind_variants(state_->bind_variants_);
}

void update_t::collect_bind_variants(bind_value_collector_t& bind_variants) const {
  for (const auto& col_op : state_->column_operations_) {
    col_op->collect_bind_variants(bind_variants);
  }
  if (state_->wheres_) {
    state_->wheres_->collect_bind_variants(bind_variants);
  }
}

std::string update_t::to_sql(const session& s, const to_sql_ctx& ctx) const {
  if (!state_->wheres_) throw std::runtime_error("WHERE condition is required for UPDATE operation");
  auto l_ctx = ctx;

  std::vector<std::string> l_set_clauses;
  for (const auto& col_op : state_->column_operations_) {
    l_set_clauses.push_back(col_op->to_sql(state_->s_, l_ctx));
  }
  auto l_sql = fmt::format(
      "UPDATE {} SET {} WHERE {}", state_->from_table_name_, fmt::join(l_set_clauses, ", "),
      state_->wheres_->to_sql(state_->s_, l_ctx)
  );
  return l_sql;
}
update_t update_t::operator()() {
  if (!state_->stmt_) {
    const to_sql_ctx l_ctx{.ctx_ = to_sql_ctx::update_sql};
    prepare(state_->s_, l_ctx);
  }
  state_->stmt_->reset_bind();
  for (const auto& val : state_->bind_variants_.bind_values_) val.bind(*state_->stmt_);

  state_->stmt_->step();
  return *this;
}
}  // namespace doodle::orm