#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/insert.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <fmt/format.h>

namespace doodle::orm {

std::string insert_t::to_sql(to_sql_ctx in_ctx) const {
  auto l_ctx = in_ctx;
  l_ctx.ctx_ |= to_sql_ctx::insert_sql;  // 强制使用 insert_sql 上下文，以确保生成正确的 SQL 片段格式
  auto l_values = fmt::format("({})", fmt::join(std::vector<std::string>(state_->columns_.size(), "?"), ", "));
  if (state_->batch_size_ > 1)
    l_values = fmt::format("{}", fmt::join(std::vector<std::string>(state_->batch_size_, l_values), ", "));
  std::vector<std::string> l_column_names{};
  for (const auto& col_info_ptr : state_->columns_) {
    l_column_names.push_back(col_info_ptr->get_column_name(*state_->s_, l_ctx));
  }
  auto l_sql =
      fmt::format("INSERT INTO {} ({}) VALUES {}", state_->into_table_name_, fmt::join(l_column_names, ", "), l_values);
  return l_sql;
}

std::int64_t insert_t::operator()() {
  if (!state_->stmt_) {
    to_sql_ctx l_ctx{.ctx_ = to_sql_ctx::insert_sql};
    auto l_sql    = to_sql(l_ctx);
    state_->stmt_ = std::make_shared<sqlite_stmt>();
    state_->stmt_->prepare(*state_->s_, l_sql);
  }

  state_->stmt_->reset_bind();
  for (const auto& val : state_->values_.bind_values_) val.bind(*state_->stmt_);
  state_->stmt_->step();
  return state_->s_->get_last_insert_rowid();
}

}  // namespace doodle::orm