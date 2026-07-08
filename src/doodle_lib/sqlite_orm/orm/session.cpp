#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/orm/session.h>

#include "storage.h"

namespace doodle::orm {
session::session_data::~session_data() {
  if (connection_ && s_) s_->add_thread_db(connection_);
}

session::session(storage& s) : data_(std::make_shared<session_data>()) {
  data_->connection_ = s.get_thread_db();
  data_->s_          = &s;
}

 
session::transaction_guard session::transaction() { return transaction_guard{*this}; }

void session::drop_table(const std::string& table_name) {
  auto l_sql  = fmt::format("DROP TABLE IF EXISTS {}", table_name);
  auto l_stmt = sqlite_stmt(*this, l_sql);
  l_stmt.step();
}
void session::drop_index(const std::string& index_name) {
  auto l_sql  = fmt::format("DROP INDEX IF EXISTS {}", index_name);
  auto l_stmt = sqlite_stmt(*this, l_sql);
  l_stmt.step();
}
void session::drop_trigger(const std::string& trigger_name) {
  auto l_sql  = fmt::format("DROP TRIGGER IF EXISTS {}", trigger_name);
  auto l_stmt = sqlite_stmt(*this, l_sql);
  l_stmt.step();
}
void session::drop_view(const std::string& view_name) {
  auto l_sql  = fmt::format("DROP VIEW IF EXISTS {}", view_name);
  auto l_stmt = sqlite_stmt(*this, l_sql);
  l_stmt.step();
}

bool session::table_exists(const std::string& table_name) {
  if (table_name.empty()) throw std::invalid_argument("Table name cannot be empty");
  using namespace detail;
  return select(*this)
      .columns(&sqlite_master_entry::name)
      .from<sqlite_master_entry>()
      .where(c(&sqlite_master_entry::type) == "table" && c(&sqlite_master_entry::name) == table_name)()
      .to_optional()
      .has_value();
}

bool session::index_exists(const std::string& index_name) {
  if (index_name.empty()) throw std::invalid_argument("Index name cannot be empty");
  using namespace detail;

  return select(*this)
      .columns(&sqlite_master_entry::name)
      .from<sqlite_master_entry>()
      .where(c(&sqlite_master_entry::type) == "index" && c(&sqlite_master_entry::name) == index_name)()
      .to_optional()
      .has_value();
}

bool session::trigger_exists(const std::string& trigger_name) {
  if (trigger_name.empty()) throw std::invalid_argument("Trigger name cannot be empty");
  using namespace detail;
  return select(*this)
      .columns(&sqlite_master_entry::name)
      .from<sqlite_master_entry>()
      .where(c(&sqlite_master_entry::type) == "trigger" && c(&sqlite_master_entry::name) == trigger_name)()
      .to_optional()
      .has_value();
}

void session::vacuum() {
  auto l_sql  = "VACUUM;";
  auto l_stmt = sqlite_stmt(*this, l_sql);
  l_stmt.step();
}
void session::exec(std::string_view sql) {
  auto l_stmt = sqlite_stmt(*this, std::string(sql));
  l_stmt.step();
}

session::transaction_guard::transaction_guard(session& s) : connection_(s.data_->connection_) { begin(); }

void session::transaction_guard::begin() {
  sqlite_stmt l_stmt{};
  l_stmt.prepare(connection_, "BEGIN TRANSACTION;");
  l_stmt.step();
}

void session::transaction_guard::commit() {
  if (committed_) throw std::runtime_error("Transaction already committed");
  sqlite_stmt l_stmt{};
  l_stmt.prepare(connection_, "COMMIT;");
  l_stmt.step();
  committed_ = true;
}
void session::transaction_guard::rollback() {
  if (committed_) throw std::runtime_error("Transaction already committed");
  sqlite_stmt l_stmt{};
  l_stmt.prepare(connection_, "ROLLBACK;");
  l_stmt.step();
  committed_ = true;
}
session::transaction_guard::~transaction_guard() {
  if (!committed_) {
    try {
      rollback();
    } catch (const std::exception& e) {
      // 在析构函数中抛出异常是危险的，因为它可能在栈展开过程中被调用
      // 这里我们选择捕获异常并记录日志，而不是让异常传播
      SPDLOG_ERROR("Failed to rollback transaction in destructor: {}", e.what());
    }
  }
}

}  // namespace doodle::orm