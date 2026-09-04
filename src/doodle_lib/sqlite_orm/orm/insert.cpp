#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/insert.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <fmt/format.h>
#include <string>
#include <vector>

namespace doodle::orm {

void insert_t::prepare(session& in_s, const to_sql_ctx& ctx) {
  if ((ctx.ctx_ & to_sql_ctx::insert_sql) && state_->is_batch_insert_ && state_->batch_insert_row_count_ == 0) {
    auto l_col_count                = state_->columns_.size();
    auto l_total_rows               = static_cast<std::int32_t>(state_->values_.bind_values_.size() / l_col_count);
    auto l_max_rows                 = std::max<std::int32_t>(1, 500 / l_col_count);
    state_->batch_insert_row_count_ = std::min(l_total_rows, l_max_rows);
  }
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
    if (!state_->is_batch_insert_) {
      l_values = fmt::format("({})", fmt::join(std::vector<std::string>(state_->columns_.size(), "?"), ", "));
    } else {
      auto l_col_count       = state_->columns_.size();
      auto l_row_placeholder = fmt::format("({})", fmt::join(std::vector<std::string>(l_col_count, "?"), ", "));
      std::vector<std::string> l_row_placeholders(state_->batch_insert_row_count_, l_row_placeholder);
      l_values = fmt::format("{}", fmt::join(l_row_placeholders, ", "));
    }
  } else if (l_ctx.ctx_ & to_sql_ctx::create_trigger_sql) {
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
  if (state_->executed_) throw std::runtime_error{"insert_t already executed"};
  if (!state_->stmt_) prepare(state_->s_, to_sql_ctx{.ctx_ = to_sql_ctx::insert_sql});

  if (!state_->is_batch_insert_) {
    state_->stmt_->reset_bind();
    for (const auto& val : state_->values_.bind_values_) val.bind(*state_->stmt_);
    state_->stmt_->step();
    state_->executed_ = true;
    return state_->stmt_->get_last_insert_rowid();
  } else {
    auto l_batch_size = state_->columns_.size() * state_->batch_insert_row_count_;
    for (auto l_view : state_->values_.bind_values_ | ranges::views::chunk(l_batch_size)) {
      // 如果末尾的数据不足一批，则直接走调整 batch_insert_row_count_ 的大小后, 重新生成sql语句, 重新prepare stmt,
      // 再执行
      if (l_view.size() != l_batch_size) {
        state_->batch_insert_row_count_ = l_view.size() / state_->columns_.size();
        prepare(state_->s_, to_sql_ctx{.ctx_ = to_sql_ctx::insert_sql});
      }
      state_->stmt_->reset_bind();
      for (const auto& val : l_view) val.bind(*state_->stmt_);
      state_->stmt_->step();
    }
    state_->executed_ = true;
    return 0;  // 由于是批量插入, 这里返回0, 因为无法返回正确的插入id
  }
}

insert_t::operator bool() const {
  return state_ && state_->executed_ && !state_->columns_.empty() && !state_->into_table_name_.empty();
}

}  // namespace doodle::orm