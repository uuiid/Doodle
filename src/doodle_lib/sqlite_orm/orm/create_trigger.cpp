#include <doodle_lib/sqlite_orm/orm/create_trigger.h>
#include <doodle_lib/sqlite_orm/orm/delete.h>
#include <doodle_lib/sqlite_orm/orm/insert.h>
#include <doodle_lib/sqlite_orm/orm/update.h>

#include <string>
#include <vector>

namespace doodle::orm {
create_trigger_t& create_trigger_t::statement(const update_t& in_statement) {
  info_->statement_ = std::move(in_statement.to_sql(to_sql_ctx{.ctx_ = to_sql_ctx::create_trigger_sql}));
  return *this;
}
create_trigger_t& create_trigger_t::statement(const delete_t& in_statement) {
  info_->statement_ = std::move(in_statement.to_sql(to_sql_ctx{.ctx_ = to_sql_ctx::create_trigger_sql}));
  return *this;
}
create_trigger_t& create_trigger_t::statement(const insert_t& in_statement) {
  info_->statement_ = std::move(in_statement.to_sql(to_sql_ctx{.ctx_ = to_sql_ctx::create_trigger_sql}));
  return *this;
}

std::string create_trigger_t::to_sql(storage& s, const to_sql_ctx& ctx) const {
  std::vector<std::string> l_column_names_str{};
  for (const auto& column_name_ptr : info_->columns_) {
    l_column_names_str.push_back(column_name_ptr->get_column_name(s, ctx));
  }
  auto l_create_trigger_sql = fmt::format(
      "CREATE TRIGGER IF NOT EXISTS {} {} {} {} ON {} BEGIN {}; END;", info_->name_, info_->timing_, info_->event_,
      fmt::format("{}", fmt::join(l_column_names_str, ", ")), info_->table_name_->get_table_name(s), info_->statement_
  );
  return l_create_trigger_sql;
}

}  // namespace doodle::orm