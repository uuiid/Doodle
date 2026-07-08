#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/insert.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <fmt/format.h>
#include <string>
#include <vector>

namespace doodle::orm {

void insert_t::prepare(session& in_s, const to_sql_ctx& ctx) {
  auto l_sql    = to_sql(in_s, ctx);
  state_->stmt_ = std::make_shared<sqlite_stmt>();
  state_->stmt_->prepare(in_s, l_sql);
  collect_bind_variants(state_->values_);
}

void insert_t::collect_bind_variants(bind_value_collector_t& bind_variants) const {}

std::string insert_t::to_sql(const session& in_s, const to_sql_ctx& in_ctx) const {
  auto l_ctx = in_ctx;
  std::string l_values{};
  if (l_ctx.ctx_ & to_sql_ctx::insert_sql) {
    l_values = fmt::format("({})", fmt::join(std::vector<std::string>(state_->columns_.size(), "?"), ", "));
    if (state_->batch_size_ > 1)
      l_values = fmt::format("{}", fmt::join(std::vector<std::string>(state_->batch_size_, l_values), ", "));
  } else if (l_ctx.ctx_ & to_sql_ctx::create_trigger_sql) {
    if (state_->batch_size_ > 1) throw std::runtime_error("Batch insert is not supported in trigger statement");
    std::vector<std::string> l_bind_values_strs{};
    for (const auto& bind_value : state_->values_.bind_values_)
      l_bind_values_strs.push_back(bind_value.to_string(in_s, l_ctx));

    l_values = fmt::format("({})", fmt::join(l_bind_values_strs, ", "));
  } else {
    throw std::runtime_error("Unsupported SQL context for insert_t::to_sql");
  }

  std::vector<std::string> l_column_names{};
  for (const auto& col_info_ptr : state_->columns_) {
    l_column_names.push_back(col_info_ptr->get_column_name(in_s, l_ctx));
  }
  auto l_sql =
      fmt::format("INSERT INTO {} ({}) VALUES {}", state_->into_table_name_, fmt::join(l_column_names, ", "), l_values);
  return l_sql;
}

std::int64_t insert_t::operator()() {
  if (!state_->stmt_) prepare(state_->s_, to_sql_ctx{.ctx_ = to_sql_ctx::insert_sql});

  state_->stmt_->reset_bind();
  for (const auto& val : state_->values_.bind_values_) val.bind(*state_->stmt_);
  state_->stmt_->step();
  return state_->stmt_->get_last_insert_rowid();
}

}  // namespace doodle::orm