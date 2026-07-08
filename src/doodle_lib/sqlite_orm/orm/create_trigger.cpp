#include <doodle_lib/sqlite_orm/orm/create_trigger.h>
#include <doodle_lib/sqlite_orm/orm/delete.h>
#include <doodle_lib/sqlite_orm/orm/insert.h>
#include <doodle_lib/sqlite_orm/orm/update.h>

#include <string>
#include <vector>

namespace doodle::orm {
create_trigger_t& create_trigger_t::statement(const update_t& in_statement) {
  info_->statement_ = std::make_shared<update_t>(in_statement);
  return *this;
}
create_trigger_t& create_trigger_t::statement(const delete_t& in_statement) {
  info_->statement_ = std::make_shared<delete_t>(in_statement);
  return *this;
}
create_trigger_t& create_trigger_t::statement(const insert_t& in_statement) {
  info_->statement_ = std::make_shared<insert_t>(in_statement);
  return *this;
}

std::string create_trigger_t::to_sql(session& in_s, const to_sql_ctx& ctx) const {
  auto l_ctx = ctx;
  l_ctx.ctx_ |= to_sql_ctx::create_trigger_sql;  // 强制使用 create_trigger_sql 上下文，以确保生成正确的 SQL 片段格式
  std::vector<std::string> l_column_names_str{};
  for (const auto& column_name_ptr : info_->columns_) {
    l_column_names_str.push_back(column_name_ptr->get_column_name(in_s, ctx));
  }
  auto l_create_trigger_sql = fmt::format(
      "CREATE TRIGGER IF NOT EXISTS {} {} {} {} ON {} BEGIN {}; END;", info_->name_, info_->timing_, info_->event_,
      fmt::format("{}", fmt::join(l_column_names_str, ", ")), info_->table_name_->to_sql(in_s, ctx),
      info_->statement_->to_sql(in_s, ctx)
  );
  return l_create_trigger_sql;
}

}  // namespace doodle::orm