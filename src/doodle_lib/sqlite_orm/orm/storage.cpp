#include "storage.h"

#include <doodle_lib/core/global_function.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/create_index.h>
#include <doodle_lib/sqlite_orm/orm/create_trigger.h>
#include <doodle_lib/sqlite_orm/orm/exception.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/sqlite_statement.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>
#include <doodle_lib/sqlite_orm/orm/storage_impl.h>

#include <boost/lockfree/detail/uses_optional.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/scope/scope_exit.hpp>

#include <chrono>
#include <fmt/format.h>
#include <mutex>
#include <set>
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <vector>

namespace doodle {
namespace orm {

/////////////////////////////////////////////////////////////////////////////////////////////////
default_value::default_value(std::string value) : value_(std::move(value)) {}
/////////////////////////////////////////////////////////////////////////////////////////////////
column_info& table_info_base::find_column_info(const table_columns_t& in_column) {
  auto l_iter = std::find_if(columns_.begin(), columns_.end(), [&in_column](const column_info& in_column_) {
    return in_column_.ptr_ == in_column;
  });
  if (l_iter == columns_.end()) throw std::runtime_error("Column not found for the given member pointer");

  return *l_iter;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

table_fts_info& table_fts_info::tokenizer(const std::string& tokenizer) {
  tokenizer_ = tokenizer;
  return *this;
}

std::string table_fts_info::to_sql(storage& s, const to_sql_ctx& ctx) const {
  std::vector<std::string> l_column_sqls;
  for (std::size_t i = 0; i < columns_.size(); ++i) {
    const auto& column = columns_[i];
    std::string l_sql  = column.name_;
    if (unindexed_columns_[i]) {
      l_sql += " UNINDEXED";
    }
    l_column_sqls.push_back(std::move(l_sql));
  }
  if (content_table_) l_column_sqls.push_back(fmt::format(R"(content='{}')", content_table_->to_sql(s, ctx)));

  if (content_rowid_)
    l_column_sqls.push_back(fmt::format(R"(content_rowid='{}')", content_rowid_->get_column_name(s, ctx)));

  if (!tokenizer_.empty()) l_column_sqls.push_back(fmt::format(R"(tokenize='{}')", tokenizer_));
  return fmt::format("CREATE VIRTUAL TABLE IF NOT EXISTS {} USING fts5 ({})", name_, fmt::join(l_column_sqls, ", "));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

table_info& table_info::add_index(const create_index_base_t& in_index) {
  indexes_.push_back(std::make_shared<create_index_base_t>(in_index));
  return *this;
}

std::string table_info::to_sql(storage& s, const to_sql_ctx& ctx) const {
  std::vector<std::string> l_column_sqls;
  for (const auto& column : columns_) {
    std::string l_sql = fmt::format(R"("{}" {})", column.name_, column.type_);
    if (column.primary_key_) {
      l_sql += " PRIMARY KEY";
    }
    if (column.autoincrement_) {
      l_sql += " AUTOINCREMENT";
    }
    if (column.not_null_) {
      l_sql += " NOT NULL";
    }
    if (column.unique_) {
      l_sql += " UNIQUE";
    }
    l_column_sqls.push_back(std::move(l_sql));
  }
  auto l_fk_sqls = get_foreign_key_create_sql(s, ctx);
  l_column_sqls.insert(l_column_sqls.end(), l_fk_sqls.begin(), l_fk_sqls.end());
  return fmt::format("CREATE TABLE IF NOT EXISTS {} ({})", name_, fmt::join(l_column_sqls, ", "));
}

/////////////////////////////////////////////////////////////////////////////////////////////////
on_delete::on_delete(foreign_key_action action) : action_(action) {}

on_update::on_update(foreign_key_action action) : action_(action) {}

std::vector<std::string> table_info_base::get_foreign_key_create_sql(storage& s, const to_sql_ctx& ctx) const {
  std::vector<std::string> l_sqls;
  for (const auto& fk : foreign_keys_) {
    std::string l_sql = fmt::format(
        "FOREIGN KEY({}) REFERENCES {}({}) ON DELETE {} ON UPDATE {}", fk.ptr_->get_column_name(s, ctx),
        fk.ref_table_->to_sql(s, ctx), fk.ref_ptr_->get_column_name(s, ctx), fk.on_delete_, fk.on_update_
    );
    l_sqls.push_back(std::move(l_sql));
  }
  return l_sqls;
}

sqlite_connection_guard_t::sqlite_connection_guard_t(storage& s) : s_(&s), connection_(s.get_thread_db()) {}

sqlite_connection_guard_t::~sqlite_connection_guard_t() {
  if (connection_ && s_) {
    // 退回池中
    s_->add_thread_db(connection_);
  }
}
sqlite_stmt::sqlite_stmt(storage& db, const std::string& sql) { prepare(db, sql); }

void sqlite_stmt::reset_bind() {
  if (bind_index_ == 0) return;  // 如果绑定索引已经是初始值，则不需要重置
  bind_index_ = 0;
  auto l_r    = sqlite3_reset(stmt_);
  DOODLE_ORM_ERROR_SQLITE3(l_r, db_.connection_->db_);
  l_r = sqlite3_clear_bindings(stmt_);
  DOODLE_ORM_ERROR_SQLITE3(l_r, db_.connection_->db_);
}

std::int32_t sqlite_stmt::get_bind_index() {
  ++bind_index_;
  return bind_index_;
}
void sqlite_stmt::prepare(storage& s, const std::string& sql) {
  if (stmt_) throw std::runtime_error("Statement already prepared");
  bind_index_ = 0;
  db_         = sqlite_connection_guard_t{s};
#ifndef NDEBUG
  SPDLOG_DEBUG("Preparing SQL statement: {}", sql);
#endif

  auto l_r = sqlite3_prepare_v2(db_.connection_->db_, sql.c_str(), sql.size(), &stmt_, nullptr);
  DOODLE_ORM_ERROR_SQLITE3(l_r, db_.connection_->db_);
}
std::int64_t sqlite_stmt::get_column_count() const { return sqlite3_column_count(stmt_); }
bool sqlite_stmt::column_is_null(int columnIndex) const {
  return sqlite3_column_type(stmt_, columnIndex) == SQLITE_NULL;
}

void sqlite_stmt::step() {
  auto l_r = sqlite3_step(stmt_);
  DOODLE_ORM_ERROR_SQLITE3(l_r, db_.connection_->db_);
}

std::int32_t sqlite_stmt::step_not_throw() { return sqlite3_step(stmt_); }
std::int64_t sqlite_stmt::get_last_insert_rowid() const { return sqlite3_last_insert_rowid(db_.connection_->db_); }

sqlite_stmt::~sqlite_stmt() { sqlite3_finalize(stmt_); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
storage::transaction_guard::transaction_guard(storage& s) : s_(s) { s_.begin_transaction(); }
void storage::transaction_guard::commit() {
  if (committed_) throw std::runtime_error("Transaction already committed");
  s_.commit_transaction();
  committed_ = true;
}
void storage::transaction_guard::rollback() {
  if (committed_) throw std::runtime_error("Transaction already committed");
  s_.rollback_transaction();
  committed_ = true;
}
storage::transaction_guard::~transaction_guard() {
  if (!committed_) {
    try {
      s_.rollback_transaction();
    } catch (const std::exception& e) {
      // 在析构函数中抛出异常是危险的，因为它可能在栈展开过程中被调用
      // 这里我们选择捕获异常并记录日志，而不是让异常传播
      SPDLOG_ERROR("Failed to rollback transaction in destructor: {}", e.what());
    }
  }
}

namespace {
void sqlite_database_error_log_callback(void* pArg, int iErrCode, const char* zMsg) {
  if (auto l_logger = static_cast<spdlog::logger*>(pArg); l_logger)
    l_logger->error(fmt::format("{} {}", iErrCode, zMsg));
}
}  // namespace

namespace {
// 表 sqlite_master 对应的结构体，用于查询数据库对象是否存在
struct sqlite_master_entry {
  std::string type;
  std::string name;
  std::string tbl_name;
  std::int32_t rootpage;
  std::string sql;
};

void reg_sqlite_master_entry(storage& s) {
  s.reg_table<sqlite_master_entry>("sqlite_master")
      .add_column("type", &sqlite_master_entry::type)
      .add_column("name", &sqlite_master_entry::name)
      .add_column("tbl_name", &sqlite_master_entry::tbl_name)
      .add_column("rootpage", &sqlite_master_entry::rootpage)
      .add_column("sql", &sqlite_master_entry::sql);
}
}  // namespace
storage::backup_t::backup_t(sqlite3* dest_db, sqlite_connection_guard_t src_db)
    : dest_db_(dest_db), src_db_(std::move(src_db)) {}
std::int32_t storage::backup_t::step(int pages) {
  if (!backup_) {
    backup_ = sqlite3_backup_init(dest_db_, "main", src_db_.connection_->db_, "main");
    if (!backup_) {
      auto l_msg = sqlite3_errmsg(dest_db_);
      if (dest_db_) sqlite3_close_v2(dest_db_);
      throw_exception(doodle_error{fmt::format("Failed to initialize backup: {}", l_msg)});
    }
  }
  auto l_r = sqlite3_backup_step(backup_, pages);
  if (l_r != SQLITE_OK && l_r != SQLITE_DONE && l_r != SQLITE_BUSY && l_r != SQLITE_LOCKED) {
    auto l_msg = sqlite3_errmsg(dest_db_);
    throw_exception(doodle_error{fmt::format("Failed to perform backup step: {}", l_msg)});
  }
  return l_r;
}
storage::backup_t::~backup_t() {
  if (backup_) sqlite3_backup_finish(backup_);
  if (dest_db_) sqlite3_close_v2(dest_db_);
}

sqlite3* storage::only_open_db() {
  // 立即打开数据库连接，确保在注册表结构时数据库已经打开
  auto l_str    = db_path_.generic_string();
  sqlite3* l_db = nullptr;
  auto l_r      = ::sqlite3_open_v2(l_str.c_str(), &l_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  DOODLE_ORM_ERROR_SQLITE3(l_r, l_db);
  // 启用扩展错误码，以便在发生错误时获取更详细的错误信息
  l_r = ::sqlite3_extended_result_codes(l_db, 1);
  DOODLE_ORM_ERROR_SQLITE3(l_r, l_db);
  register_custom_extension(l_db);
  return l_db;
}

sqlite_connection_ptr storage::get_thread_db() {
  constexpr auto kMaxIdleTime = std::chrono::minutes{5};  // 最大空闲时间，超过这个时间的连接将被关闭
  timed_connection l_entry{};

  // 循环出队，跳过已超时的空闲连接（惰性回收）
  while (connection_queue_.pop(l_entry)) {
    --thread_db_count_;
    const auto l_idle = std::chrono::steady_clock::now() - l_entry.idle_since_;
    if (l_idle < kMaxIdleTime) {
      return std::move(l_entry.conn_);
    }
    // 超时 → shared_ptr 析构，自动 sqlite3_close
    SPDLOG_LOGGER_DEBUG(
        g_logger_ctrl().get_main_error(), "回收空闲数据库连接 (空闲 {} 秒)",
        std::chrono::duration_cast<std::chrono::seconds>(l_idle).count()
    );
  }

  // 池中无可用连接或全部超时 → 新建
  sqlite_connection_ptr l_connection;
  if (!l_entry.conn_) {
    l_connection = std::make_shared<sqlite_connection_t>(only_open_db());
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_main_error(), "创建了一个新的线程数据库连接，当前池中连接数: {}", thread_db_count_ + 1
    );
  } else
    l_connection = std::move(l_entry.conn_);
  return l_connection;
}

void storage::add_thread_db(const sqlite_connection_ptr& in_ptr) {
  timed_connection l_entry{in_ptr, std::chrono::steady_clock::now()};
  if (connection_queue_.push(std::move(l_entry)))
    ++thread_db_count_;
  else {
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_main_error(), "线程数据库连接池已满，当前池中连接数: {}",
        boost::numeric_cast<std::int32_t>(thread_db_count_.load())
    );
  }
}

void storage::open_(FSys::path in_path, std::int32_t in_flags) {
  static std::once_flag l_flag{};
  std::call_once(l_flag, []() {
    sqlite3_config(SQLITE_CONFIG_LOG, sqlite_database_error_log_callback, spdlog::default_logger_raw());
  });

  if (in_path.empty()) in_path = ":memory:";
  db_path_ = in_path.generic_string();
}

void storage::open(const FSys::path& in_path) { open_(in_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE); }

void storage::open(FSys::path in_path, std::int32_t in_flags) { open_(in_path, in_flags); }

void storage::open() { open_({}, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE); }

create_trigger_t storage::create_trigger(std::string in_name) {
  auto l_trigger = std::make_shared<create_trigger_t>(std::move(in_name));
  triggers_.emplace_back(l_trigger);
  return *l_trigger;
}

fts5_api* storage::get_fts5_api(sqlite3* in_sqlite) {
  DOODLE_CHICK(in_sqlite != nullptr, "Database not opened");
  fts5_api* pRet      = nullptr;
  sqlite3_stmt* pStmt = nullptr;
  boost::scope::scope_exit stmt_guard([&]() {
    if (pStmt) sqlite3_finalize(pStmt);
  });

  if (SQLITE_OK == sqlite3_prepare_v2(in_sqlite, "SELECT fts5(?1)", -1, &pStmt, nullptr)) {
    sqlite3_bind_pointer(pStmt, 1, (void*)&pRet, "fts5_api_ptr", nullptr);
    sqlite3_step(pStmt);
  } else {
    auto l_msg = sqlite3_errmsg(in_sqlite);
    SPDLOG_ERROR("Failed to prepare statement: {}", l_msg);
    throw_exception(doodle_error{l_msg});
  }
  return pRet;
}

void storage::sync_schema() {
  if (!has_reg_table<sqlite_master_entry>()) reg_sqlite_master_entry(*this);

  auto l_transaction = transaction();
  for (const auto& table : tables_) {
    if (table_exists(table->name_)) {
      SPDLOG_DEBUG("Table already exists, skipping creation: {}", table->name_);
      continue;
    }
    if (table->name_ == "sqlite_master")
      // sqlite_master 是 SQLite 内部表，不能创建
      continue;

    auto l_create_table_sql = table->to_sql(*this, to_sql_ctx{.ctx_ = to_sql_ctx::create_table_sql});
    auto l_stmt             = sqlite_stmt(*this, l_create_table_sql);
    l_stmt.step();
  }
  std::set<create_index_base_t::index_info> l_existing_indexes;
  for (const auto& table : tables_) {
    for (const auto& index : table->indexes_) {
      auto l_index_info = index->get_index_info(*this, to_sql_ctx{.ctx_ = to_sql_ctx::create_index_sql});
      if (l_existing_indexes.contains(l_index_info)) {
        SPDLOG_DEBUG("Index already exists, skipping creation: {}", l_index_info.name_);
        // 已经存在相同的索引，无需创建
        continue;
      }
      if (index_exists(l_index_info.name_)) {
        SPDLOG_DEBUG("Index already exists in database, skipping creation: {}", l_index_info.name_);
        l_existing_indexes.insert(l_index_info);
        continue;
      }

      auto l_create_index_sql = index->to_sql(*this, to_sql_ctx{.ctx_ = to_sql_ctx::create_index_sql});
      auto l_stmt             = sqlite_stmt(*this, l_create_index_sql);
      l_stmt.step();
    }
  }
  for (const auto& l_trigger : triggers_) {
    if (trigger_exists(l_trigger->info_->name_)) {
      SPDLOG_DEBUG("Trigger already exists, skipping creation: {}", l_trigger->info_->name_);
      continue;
    }

    auto l_create_trigger_sql = l_trigger->to_sql(*this, to_sql_ctx{.ctx_ = to_sql_ctx::create_trigger_sql});
    auto l_stmt               = sqlite_stmt(*this, l_create_trigger_sql);
    l_stmt.step();
  }
  l_transaction.commit();
}

storage::backup_t storage::backup(const FSys::path& dest_path) {
  sqlite3* dest_db = nullptr;
  auto l_str       = dest_path.generic_string();
  auto l_r         = ::sqlite3_open_v2(l_str.c_str(), &dest_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  if (l_r != SQLITE_OK) {
    auto l_msg = sqlite3_errmsg(dest_db);
    if (dest_db) sqlite3_close_v2(dest_db);
    throw_exception(doodle_error{fmt::format("Failed to open destination database: {}", l_msg)});
  }
  return backup_t(dest_db, sqlite_connection_guard_t{*this});
}

void storage::pragma_t::synchronous(std::int32_t in_sync) {
  if (in_sync < 0 || in_sync > 2) throw std::invalid_argument("Invalid synchronous value, must be 0, 1, or 2");
  run("synchronous", in_sync);
}
void storage::pragma_t::journal_mode(journal_mode_t in_mode) {
  static const std::map<journal_mode_t, std::string_view> mode_to_str{
      {journal_mode_t::delete_, "DELETE"}, {journal_mode_t::truncate, "TRUNCATE"}, {journal_mode_t::persist, "PERSIST"},
      {journal_mode_t::memory, "MEMORY"},  {journal_mode_t::wal, "WAL"},           {journal_mode_t::off, "OFF"},
  };
  if (!mode_to_str.contains(in_mode)) throw std::invalid_argument("Invalid journal mode");
  run("journal_mode", mode_to_str.at(in_mode));
}
void storage::pragma_t::recursive_triggers(bool in_recursive) { run("recursive_triggers", in_recursive); }
void storage::pragma_t::foreign_keys(bool in_foreign_keys) { run("foreign_keys", in_foreign_keys); }
void storage::pragma_t::locking_mode(bool in_exclusive) { run("locking_mode", in_exclusive ? "EXCLUSIVE" : "NORMAL"); }
void storage::pragma_t::user_version(std::int32_t version) { run("user_version", version); }
std::int32_t storage::pragma_t::user_version() {
  sqlite_stmt l_stmt{};
  l_stmt.prepare(s_, "PRAGMA user_version;");
  l_stmt.step();
  return l_stmt.get_column_value<std::int32_t>(0);
}

void storage::pragma_t::run(std::string_view in_pragma_sql, bool in_value) {
  auto l_sql  = fmt::format("PRAGMA {} = {}", in_pragma_sql, in_value ? "ON" : "OFF");
  auto l_stmt = sqlite_stmt(s_, l_sql);
  l_stmt.step();
}

void storage::pragma_t::run(std::string_view in_pragma_sql, std::string_view in_value) {
  auto l_sql  = fmt::format("PRAGMA {} = {}", in_pragma_sql, in_value);
  auto l_stmt = sqlite_stmt(s_, l_sql);
  l_stmt.step();
}
void storage::pragma_t::run(std::string_view in_pragma_sql, std::int32_t in_value) {
  auto l_sql  = fmt::format("PRAGMA {} = {}", in_pragma_sql, in_value);
  auto l_stmt = sqlite_stmt(s_, l_sql);
  l_stmt.step();
}

storage::~storage() = default;
void storage::register_custom_extension(sqlite3* in_sqlite) {}

std::string storage::get_table_name(std::type_index in_type_index) const {
  if (!type_to_table_index_.contains(in_type_index)) {
    throw std::runtime_error("Table not found for the given type");
  }
  auto l_table_index = type_to_table_index_.at(in_type_index);
  return tables_[l_table_index]->name_;
}

void storage::begin_transaction() {
  auto l_stmt = sqlite_stmt{*this, "BEGIN TRANSACTION;"};
  l_stmt.step();
}

void storage::commit_transaction() {
  auto l_stmt = sqlite_stmt{*this, "COMMIT;"};
  l_stmt.step();
}
void storage::rollback_transaction() {
  auto l_stmt = sqlite_stmt{*this, "ROLLBACK;"};
  l_stmt.step();
}
storage::transaction_guard storage::transaction() { return transaction_guard{*this}; }
storage::pragma_t& storage::pragma() { return pragma_; }

std::string storage::get_column_name(const table_columns_t& in_column, const to_sql_ctx& ctx) const {
  auto l_type_index = in_column.table_type_index_;
  if (!type_to_table_index_.contains(l_type_index)) throw std::runtime_error("Table not found for the given type");

  auto l_table_index = type_to_table_index_.at(l_type_index);
  auto& l_table      = static_cast<table_info&>(*tables_[l_table_index]);
  auto& l_column     = l_table.find_column_info(in_column);

  if (ctx.ctx_ & to_sql_ctx::alias_sql) return fmt::format(R"("{}")", l_column.name_);

  if ((ctx.ctx_ & to_sql_ctx::insert_sql || ctx.ctx_ & to_sql_ctx::update_sql || ctx.ctx_ & to_sql_ctx::delete_sql) &&
      !(ctx.ctx_ & to_sql_ctx::where_sql))
    return fmt::format(R"("{}")", l_column.name_);

  if (ctx.ctx_ & to_sql_ctx::select_sql || ctx.ctx_ & to_sql_ctx::where_sql)
    return fmt::format(R"("{}"."{}")", l_table.name_, l_column.name_);
  return fmt::format(R"("{}")", l_column.name_);
}

void storage::drop_table(const std::string& table_name) {
  auto l_sql  = fmt::format("DROP TABLE IF EXISTS {}", table_name);
  auto l_stmt = sqlite_stmt(*this, l_sql);
  l_stmt.step();
}
void storage::drop_index(const std::string& index_name) {
  auto l_sql  = fmt::format("DROP INDEX IF EXISTS {}", index_name);
  auto l_stmt = sqlite_stmt(*this, l_sql);
  l_stmt.step();
}
void storage::drop_trigger(const std::string& trigger_name) {
  auto l_sql  = fmt::format("DROP TRIGGER IF EXISTS {}", trigger_name);
  auto l_stmt = sqlite_stmt(*this, l_sql);
  l_stmt.step();
}
void storage::drop_view(const std::string& view_name) {
  auto l_sql  = fmt::format("DROP VIEW IF EXISTS {}", view_name);
  auto l_stmt = sqlite_stmt(*this, l_sql);
  l_stmt.step();
}

bool storage::table_exists(const std::string& table_name) {
  if (table_name.empty()) throw std::invalid_argument("Table name cannot be empty");
  if (!has_reg_table<sqlite_master_entry>()) reg_sqlite_master_entry(*this);
  return select(*this)
      .columns(&sqlite_master_entry::name)
      .from<sqlite_master_entry>()
      .where(c(&sqlite_master_entry::type) == "table" && c(&sqlite_master_entry::name) == table_name)()
      .to_optional()
      .has_value();
}

bool storage::index_exists(const std::string& index_name) {
  if (index_name.empty()) throw std::invalid_argument("Index name cannot be empty");
  if (!has_reg_table<sqlite_master_entry>()) reg_sqlite_master_entry(*this);

  return select(*this)
      .columns(&sqlite_master_entry::name)
      .from<sqlite_master_entry>()
      .where(c(&sqlite_master_entry::type) == "index" && c(&sqlite_master_entry::name) == index_name)()
      .to_optional()
      .has_value();
}

bool storage::trigger_exists(const std::string& trigger_name) {
  if (trigger_name.empty()) throw std::invalid_argument("Trigger name cannot be empty");
  if (!has_reg_table<sqlite_master_entry>()) reg_sqlite_master_entry(*this);

  return select(*this)
      .columns(&sqlite_master_entry::name)
      .from<sqlite_master_entry>()
      .where(c(&sqlite_master_entry::type) == "trigger" && c(&sqlite_master_entry::name) == trigger_name)()
      .to_optional()
      .has_value();
}

void storage::vacuum() {
  auto l_sql  = "VACUUM;";
  auto l_stmt = sqlite_stmt(*this, l_sql);
  l_stmt.step();
}
void storage::exec(std::string_view sql) {
  auto l_stmt = sqlite_stmt(*this, std::string(sql));
  l_stmt.step();
}
}  // namespace orm

}  // namespace doodle