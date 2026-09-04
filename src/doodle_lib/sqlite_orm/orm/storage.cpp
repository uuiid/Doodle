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

std::string table_fts_info::to_sql(session& s, const to_sql_ctx& ctx) const {
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

std::string table_info::to_sql(session& s, const to_sql_ctx& ctx) const {
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

std::vector<std::string> table_info_base::get_foreign_key_create_sql(session& s, const to_sql_ctx& ctx) const {
  std::vector<std::string> l_sqls;
  for (const auto& fk : foreign_keys_) {
    // 生成约束名称
    auto l_constraint_name = fmt::format(
        "fk_{}_{}_to_{}_{}", name_, fk.ptr_->get_column_name(s, ctx), fk.ref_table_->to_sql(s, ctx),
        fk.ref_ptr_->get_column_name(s, ctx)
    );
    // 清除名称中的 " 字符
    l_constraint_name.erase(
        std::remove(l_constraint_name.begin(), l_constraint_name.end(), '"'), l_constraint_name.end()
    );
    std::string l_sql = fmt::format(
        "CONSTRAINT {} FOREIGN KEY({}) REFERENCES {}({}) ON DELETE {} ON UPDATE {}", l_constraint_name,
        fk.ptr_->get_column_name(s, ctx), fk.ref_table_->to_sql(s, ctx), fk.ref_ptr_->get_column_name(s, ctx),
        fk.on_delete_, fk.on_update_
    );
    l_sqls.push_back(std::move(l_sql));
  }
  return l_sqls;
}

sqlite_stmt::sqlite_stmt(const sqlite_connection_ptr& db, const std::string& sql) { prepare(db, sql); }
sqlite_stmt::sqlite_stmt(const session& s, const std::string& sql) { prepare(s, sql); }

void sqlite_stmt::reset_bind() {
  if (bind_index_ == 0) return;  // 如果绑定索引已经是初始值，则不需要重置
  bind_index_ = 0;
  auto l_r    = sqlite3_reset(stmt_);
  DOODLE_ORM_ERROR_SQLITE3(l_r, sqlite3_db_handle(stmt_));
  l_r = sqlite3_clear_bindings(stmt_);
  DOODLE_ORM_ERROR_SQLITE3(l_r, sqlite3_db_handle(stmt_));
}

std::int32_t sqlite_stmt::get_bind_index() {
  ++bind_index_;
  return bind_index_;
}
void sqlite_stmt::prepare(const sqlite_connection_ptr& db, const std::string& sql) {
  if (stmt_) throw std::runtime_error("Statement already prepared");
  bind_index_ = 0;
#ifndef NDEBUG
  SPDLOG_DEBUG("Preparing SQL statement: {}", sql);
#endif

  auto l_r = sqlite3_prepare_v2(*db, sql.c_str(), sql.size(), &stmt_, nullptr);
  DOODLE_ORM_ERROR_SQLITE3(l_r, (*db));
}
void sqlite_stmt::prepare(const session& s, const std::string& sql) { return prepare(s.get_connection(), sql); }
std::int64_t sqlite_stmt::get_column_count() const { return sqlite3_column_count(stmt_); }
bool sqlite_stmt::column_is_null(int columnIndex) const {
  return sqlite3_column_type(stmt_, columnIndex) == SQLITE_NULL;
}

void sqlite_stmt::step() {
  auto l_r = sqlite3_step(stmt_);
  DOODLE_ORM_ERROR_SQLITE3(l_r, sqlite3_db_handle(stmt_));
}

std::int32_t sqlite_stmt::step_not_throw() { return sqlite3_step(stmt_); }
std::int64_t sqlite_stmt::get_last_insert_rowid() const { return sqlite3_last_insert_rowid(sqlite3_db_handle(stmt_)); }

sqlite_stmt::~sqlite_stmt() { sqlite3_finalize(stmt_); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {
void sqlite_database_error_log_callback(void* pArg, int iErrCode, const char* zMsg) {
  if (auto l_logger = static_cast<spdlog::logger*>(pArg); l_logger)
    l_logger->error(fmt::format("{} {}", iErrCode, zMsg));
}
}  // namespace

namespace {

void reg_sqlite_master_entry(storage& s) {
  using namespace detail;
  s.reg_table<sqlite_master_entry>("sqlite_master")
      .add_column("type", &sqlite_master_entry::type)
      .add_column("name", &sqlite_master_entry::name)
      .add_column("tbl_name", &sqlite_master_entry::tbl_name)
      .add_column("rootpage", &sqlite_master_entry::rootpage)
      .add_column("sql", &sqlite_master_entry::sql);
  s.reg_table<pragma_foreign_key_check_entry>("pragma_foreign_key_check")
      .add_column("table", &pragma_foreign_key_check_entry::table)
      .add_column("rowid", &pragma_foreign_key_check_entry::rowid)
      .add_column("parent", &pragma_foreign_key_check_entry::parent)
      .add_column("fkid", &pragma_foreign_key_check_entry::fkid);
}
}  // namespace
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
  sqlite_connection_ptr l_connection;
  // 循环出队，跳过已超时的空闲连接（惰性回收）
  while (connection_queue_.pop(l_entry)) {
    --thread_db_count_;
    const auto l_idle = std::chrono::steady_clock::now() - l_entry.idle_since_;
    if (l_idle < kMaxIdleTime) return std::move(l_entry.conn_);

    // 超时 → shared_ptr 析构，自动 sqlite3_close
    SPDLOG_LOGGER_DEBUG(
        g_logger_ctrl().get_main_error(), "回收空闲数据库连接 (空闲 {} 秒)",
        std::chrono::duration_cast<std::chrono::seconds>(l_idle).count()
    );
    // 在这里保存一个超时的链接, 以便在没有可用连接时, 不需要直接创建新的连接, 而是可以使用这个超时的连接
    l_connection = std::move(l_entry.conn_);
  }

  // 池中无可用连接或全部超时 → 新建
  if (!l_connection) {
    l_connection = std::make_shared<sqlite_connection_t>(only_open_db());
    // SPDLOG_LOGGER_WARN(
    //     g_logger_ctrl().get_main_error(), "创建了一个新的线程数据库连接，当前池中连接数: {}", thread_db_count_ + 1
    // );
  }
  return l_connection;
}

void storage::add_thread_db(const sqlite_connection_ptr& in_ptr) {
  if (auto l_last_err = sqlite3_extended_errcode(*in_ptr); l_last_err != 0) {
    // SPDLOG_LOGGER_ERROR(g_logger_ctrl().get_main_error(), "数据库连接健康检查失败, 销毁连接而非回池");
    return;  // shared_ptr 析构 → sqlite3_close
  }
  timed_connection l_entry{in_ptr, std::chrono::steady_clock::now()};
  if (connection_queue_.push(std::move(l_entry))) ++thread_db_count_;
  // else {
  //   SPDLOG_LOGGER_WARN(
  //       g_logger_ctrl().get_main_error(), "线程数据库连接池已满，当前池中连接数: {}",
  //       boost::numeric_cast<std::int32_t>(thread_db_count_.load())
  //   );
  // }
}

void storage::open_(FSys::path in_path, std::int32_t in_flags) {
  static std::once_flag l_flag{};
  std::call_once(l_flag, []() {
    sqlite3_config(SQLITE_CONFIG_LOG, sqlite_database_error_log_callback, spdlog::default_logger_raw());
  });
  if (!has_reg_table<detail::sqlite_master_entry>()) reg_sqlite_master_entry(*this);

  if (in_path.empty()) in_path = ":memory:";
  db_path_ = in_path.generic_string();
}

void storage::open(const FSys::path& in_path) {
  boost::scope::scope_exit guard([this]() { is_opened_ = false; });
  is_opened_ = true;
  open_(in_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
}

void storage::open(FSys::path in_path, std::int32_t in_flags) {
  boost::scope::scope_exit guard([this]() { is_opened_ = false; });
  is_opened_ = true;
  open_(in_path, in_flags);
}

void storage::open() {
  boost::scope::scope_exit guard([this]() { is_opened_ = false; });
  is_opened_ = true;
  open_({}, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
}

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

session storage::create_session() { return session{*this}; }

storage::~storage() = default;
void storage::register_custom_extension(sqlite3* in_sqlite) {}

std::string storage::get_table_name(std::type_index in_type_index) const {
  if (!type_to_table_index_.contains(in_type_index)) {
    throw std::runtime_error("Table not found for the given type");
  }
  auto l_table_index = type_to_table_index_.at(in_type_index);
  return tables_[l_table_index]->name_;
}

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

}  // namespace orm

}  // namespace doodle