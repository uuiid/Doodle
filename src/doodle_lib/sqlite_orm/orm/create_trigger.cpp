#include <doodle_lib/sqlite_orm/orm/create_trigger.h>
#include <doodle_lib/sqlite_orm/orm/delete.h>
#include <doodle_lib/sqlite_orm/orm/insert.h>
#include <doodle_lib/sqlite_orm/orm/update.h>

namespace doodle::orm {
create_trigger_t& create_trigger_t::statement(const update_t& in_statement) {
  info_->statement_ = std::move(in_statement.to_sql());
  return *this;
}
create_trigger_t& create_trigger_t::statement(const delete_t& in_statement) {
  info_->statement_ = std::move(in_statement.to_sql());
  return *this;
}
create_trigger_t& create_trigger_t::statement(const insert_t& in_statement) {
  info_->statement_ = std::move(in_statement.to_sql());
  return *this;
}
}  // namespace doodle::orm