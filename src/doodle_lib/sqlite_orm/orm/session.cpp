#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/orm/session.h>

#include "storage.h"

namespace doodle::orm {
session::session_data::~session_data() {
  if (connection_ && s_) s_->add_thread_db(connection_);
}

session::session(storage& s) : data_(std::make_shared<session_data>()) {
  if (!s.is_opened_) data_->connection_ = s.get_thread_db();
  data_->s_ = &s;
}
sqlite_connection_ptr session::get_connection() const {
  if (!data_ || !data_->connection_) throw std::runtime_error("Session is not connected to a database");
  return data_->connection_;
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
std::set<std::string> session::get_all_table_names() {
  using namespace detail;
  auto l_tables = select(*this)
                      .columns(&sqlite_master_entry::name)
                      .from<sqlite_master_entry>()
                      .where(c(&sqlite_master_entry::type) == "table")()
                      .to_set();
  return l_tables;
}
std::set<std::string> session::get_all_index_names() {
  using namespace detail;
  auto l_indexes = select(*this)
                       .columns(&sqlite_master_entry::name)
                       .from<sqlite_master_entry>()
                       .where(c(&sqlite_master_entry::type) == "index")()
                       .to_set();
  return l_indexes;
}

std::set<std::string> session::get_all_trigger_names() {
  using namespace detail;
  auto l_triggers = select(*this)
                        .columns(&sqlite_master_entry::name)
                        .from<sqlite_master_entry>()
                        .where(c(&sqlite_master_entry::type) == "trigger")()
                        .to_set();
  return l_triggers;
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

void session::sync_schema() {
  auto& l_s           = *data_->s_;
  auto l_all_tables   = get_all_table_names();
  auto l_all_indexes  = get_all_index_names();
  auto l_all_triggers = get_all_trigger_names();
  auto l_transaction  = transaction();
  for (const auto& table : l_s.tables_) {
    if (l_all_tables.contains(table->name_)) {
      SPDLOG_DEBUG("Table already exists, skipping creation: {}", table->name_);
      continue;
    }
    if (table->name_ == "sqlite_master") continue;

    auto l_create_table_sql = table->to_sql(*this, to_sql_ctx{.ctx_ = to_sql_ctx::create_table_sql});
    auto l_stmt             = sqlite_stmt{*this, l_create_table_sql};
    l_stmt.step();
  }
  std::set<create_index_base_t::index_info> l_existing_indexes;
  for (const auto& table : l_s.tables_) {
    for (const auto& index : table->indexes_) {
      auto l_index_info = index->get_index_info(*this, to_sql_ctx{.ctx_ = to_sql_ctx::create_index_sql});
      if (l_existing_indexes.contains(l_index_info)) {
        SPDLOG_DEBUG("Index already exists, skipping creation: {}", l_index_info.name_);
        continue;
      }
      if (l_all_indexes.contains(l_index_info.name_)) {
        SPDLOG_DEBUG("Index already exists in database, skipping creation: {}", l_index_info.name_);
        l_existing_indexes.insert(l_index_info);
        continue;
      }

      auto l_create_index_sql = index->to_sql(*this, to_sql_ctx{.ctx_ = to_sql_ctx::create_index_sql});
      auto l_stmt             = sqlite_stmt{*this, l_create_index_sql};
      l_stmt.step();
    }
  }
  for (const auto& l_trigger : l_s.triggers_) {
    if (l_all_triggers.contains(l_trigger->info_->name_)) {
      SPDLOG_DEBUG("Trigger already exists, skipping creation: {}", l_trigger->info_->name_);
      continue;
    }

    auto l_create_trigger_sql = l_trigger->to_sql(*this, to_sql_ctx{.ctx_ = to_sql_ctx::create_trigger_sql});
    auto l_stmt               = sqlite_stmt{*this, l_create_trigger_sql};
    l_stmt.step();
  }
  l_transaction.commit();
}

void session::rebuild_table(const std::type_index& table_name, const std::vector<std::string>& in_new_columns) {
  auto& l_s = *data_->s_;
  if (!l_s.type_to_table_index_.contains(table_name)) throw std::runtime_error("Table not found for the given type");
  auto l_table_index = l_s.type_to_table_index_.at(table_name);
  auto& l_old_table  = l_s.tables_[l_table_index];
  auto l_transaction = transaction();
  // 预先关闭外键约束检查，以避免在重建表时出现外键约束错误
  this->pragma().foreign_keys(false);

  {
    auto l_new_table   = l_old_table->clone();
    l_new_table->name_ = l_old_table->name_ + "_backup";
    //   创建新表, 需要包含索引
    auto l_create_sql  = l_new_table->to_sql(*this, to_sql_ctx{.ctx_ = to_sql_ctx::create_table_sql});
    //   重命名
    auto l_rename_sql  = fmt::format(R"(ALTER TABLE "{}" RENAME TO "{}";)", l_new_table->name_, l_old_table->name_);
    //   将数据从旧表复制到新表
    std::vector<std::string> l_column_names;
    for (const auto& column : l_old_table->columns_) {
      l_column_names.push_back(column.name_);
    }
    // 如何有新增的列, 则需要将 l_column_names 中去除新增的列, 只保留旧表中存在的列,
    // 这个是为了避免在复制数据时出现列不匹配的错误
    for (const auto& new_column : in_new_columns) {
      l_column_names.erase(std::remove(l_column_names.begin(), l_column_names.end(), new_column), l_column_names.end());
    }
    auto l_copy_sql = fmt::format(
        R"(INSERT INTO "{}" ("{}") SELECT "{}" FROM "{}";)", l_new_table->name_, fmt::join(l_column_names, R"(", ")"),
        fmt::join(l_column_names, R"(", ")"), l_old_table->name_
    );
    //   删除旧表
    auto l_drop_sql    = fmt::format(R"(DROP TABLE "{}";)", l_old_table->name_);
    // 执行 SQL 语句
    auto l_create_stmt = sqlite_stmt{*this, l_create_sql};
    l_create_stmt.step();
    auto l_copy_stmt = sqlite_stmt{*this, l_copy_sql};
    l_copy_stmt.step();
    auto l_drop_stmt = sqlite_stmt{*this, l_drop_sql};
    l_drop_stmt.step();
    auto l_rename_stmt = sqlite_stmt{*this, l_rename_sql};
    l_rename_stmt.step();
    auto l_all_triggers = get_all_trigger_names();
    // 5. 创建索引和触发器
    for (const auto& index : l_old_table->indexes_) {
      auto l_create_index_sql = index->to_sql(*this, to_sql_ctx{.ctx_ = to_sql_ctx::create_index_sql});
      auto l_stmt             = sqlite_stmt{*this, l_create_index_sql};
      l_stmt.step();
    }
    for (const auto& trigger : l_s.triggers_) {
      if (l_all_triggers.contains(trigger->info_->name_)) {
        SPDLOG_DEBUG("Trigger already exists, skipping creation: {}", trigger->info_->name_);
        continue;
      }
      auto l_create_trigger_sql = trigger->to_sql(*this, to_sql_ctx{.ctx_ = to_sql_ctx::create_trigger_sql});
      auto l_stmt               = sqlite_stmt{*this, l_create_trigger_sql};
      l_stmt.step();
    }
  }
  l_transaction.commit();
  // 重新启用外键约束检查(如何失败, 链接将被抛弃, 无需进行回滚)
  this->pragma().foreign_keys(true);
}

void session::rename_table(const std::string& old_name, const std::string& new_name) {
  if (old_name.empty() || new_name.empty()) throw std::invalid_argument("Table names cannot be empty");
  auto l_sql  = fmt::format(R"(ALTER TABLE "{}" RENAME TO "{}";)", old_name, new_name);
  auto l_stmt = sqlite_stmt(*this, l_sql);
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

void session::pragma_t::synchronous(std::int32_t in_sync) {
  if (in_sync < 0 || in_sync > 2) throw std::invalid_argument("Invalid synchronous value, must be 0, 1, or 2");
  run("synchronous", in_sync);
}
void session::pragma_t::journal_mode(journal_mode_t in_mode) {
  static const std::map<journal_mode_t, std::string_view> mode_to_str{
      {journal_mode_t::delete_, "DELETE"}, {journal_mode_t::truncate, "TRUNCATE"}, {journal_mode_t::persist, "PERSIST"},
      {journal_mode_t::memory, "MEMORY"},  {journal_mode_t::wal, "WAL"},           {journal_mode_t::off, "OFF"},
  };
  if (!mode_to_str.contains(in_mode)) throw std::invalid_argument("Invalid journal mode");
  run("journal_mode", mode_to_str.at(in_mode));
}
void session::pragma_t::recursive_triggers(bool in_recursive) { run("recursive_triggers", in_recursive); }
void session::pragma_t::foreign_keys(bool in_foreign_keys) { run("foreign_keys", in_foreign_keys); }
void session::pragma_t::locking_mode(bool in_exclusive) { run("locking_mode", in_exclusive ? "EXCLUSIVE" : "NORMAL"); }
void session::pragma_t::user_version(std::int32_t version) { run("user_version", version); }
std::int32_t session::pragma_t::user_version() {
  sqlite_stmt l_stmt{};
  l_stmt.prepare(s_, "PRAGMA user_version;");
  l_stmt.step();
  return l_stmt.get_column_value<std::int32_t>(0);
}

void session::pragma_t::run(std::string_view in_pragma_sql, bool in_value) {
  auto l_sql  = fmt::format("PRAGMA {} = {}", in_pragma_sql, in_value ? "ON" : "OFF");
  auto l_stmt = sqlite_stmt(s_, l_sql);
  l_stmt.step();
}

void session::pragma_t::run(std::string_view in_pragma_sql, std::string_view in_value) {
  auto l_sql  = fmt::format("PRAGMA {} = {}", in_pragma_sql, in_value);
  auto l_stmt = sqlite_stmt(s_, l_sql);
  l_stmt.step();
}
void session::pragma_t::run(std::string_view in_pragma_sql, std::int32_t in_value) {
  auto l_sql  = fmt::format("PRAGMA {} = {}", in_pragma_sql, in_value);
  auto l_stmt = sqlite_stmt(s_, l_sql);
  l_stmt.step();
}

}  // namespace doodle::orm