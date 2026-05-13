#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/exception.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/sqlite_statement.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>
#include <doodle_lib/sqlite_orm/orm/storage_impl.h>

#include <fmt/format.h>
#include <sqlite3.h>
#include <string>
#include <vector>

namespace doodle {
namespace orm {

void sqlite_stmt::reset_bind() {
  if (bind_index_ == 0) return;  // 如果绑定索引已经是初始值，则不需要重置
  bind_index_ = 0;
  auto l_r    = sqlite3_reset(stmt_);
  DOODLE_ORM_ERROR_SQLITE3(l_r, db_);
  l_r = sqlite3_clear_bindings(stmt_);
  DOODLE_ORM_ERROR_SQLITE3(l_r, db_);
}

std::int32_t sqlite_stmt::get_bind_index() {
  ++bind_index_;
  return bind_index_;
}
void sqlite_stmt::prepare(storage& s, const std::string& sql) {
  if (stmt_) throw std::runtime_error("Statement already prepared");
  bind_index_ = 0;
  db_         = s.db_;
  auto l_r    = sqlite3_prepare_v2(db_, sql.c_str(), sql.size(), &stmt_, nullptr);
  DOODLE_ORM_ERROR_SQLITE3(l_r, db_);
}
std::int64_t sqlite_stmt::get_column_count() const { return sqlite3_column_count(stmt_); }

std::int32_t sqlite_stmt::bind(const storage_column_variant& value) {
  return std::visit(
      [this](auto&& arg) -> std::int32_t {
        using T = std::decay_t<decltype(arg)>;
        return sqlite_statement_binder<T>{}.bind(stmt_, get_bind_index(), arg);
      },
      value
  );
}

void sqlite_stmt::step() {
  auto l_r = sqlite3_step(stmt_);
  DOODLE_ORM_ERROR_SQLITE3(l_r, db_);
}

std::int32_t sqlite_stmt::step_not_throw() { return sqlite3_step(stmt_); }

sqlite_stmt::~sqlite_stmt() { sqlite3_finalize(stmt_); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void storage::open(FSys::path in_path, std::int32_t in_flags) {
  if (in_path.empty()) in_path = ":memory:";
  db_path_ = std::move(in_path);
  db_path_.make_preferred();
  auto l_str = db_path_.generic_string();
  // 立即打开数据库连接，确保在注册表结构时数据库已经打开
  auto l_r   = ::sqlite3_open_v2(l_str.c_str(), &db_, in_flags, nullptr);
  DOODLE_ORM_ERROR_SQLITE3(l_r, db_);
  // 启用扩展错误码，以便在发生错误时获取更详细的错误信息
  l_r = ::sqlite3_extended_result_codes(db_, 1);
  DOODLE_ORM_ERROR_SQLITE3(l_r, db_);
}

void storage::sync_schema() {
  for (const auto& table : tables_) {
    auto l_create_table_sql = table->get_table_create_sql();
    auto l_stmt             = sqlite_stmt(*this, l_create_table_sql);
    l_stmt.step();
  }
  for (const auto& index : indexes_) {
    auto l_create_index_sql =
        fmt::format("CREATE INDEX IF NOT EXISTS {} ON {} ({})", index.name_, index.table_name_, index.column_name_);
    auto l_stmt = sqlite_stmt(*this, l_create_index_sql);
    l_stmt.step();
  }
  for (const auto& unique_index : unique_indexes_) {
    std::vector<std::string> column_names;
    auto l_create_unique_index_sql = fmt::format(
        "CREATE UNIQUE INDEX IF NOT EXISTS {} ON {} ({})", unique_index.name_, unique_index.table_name_,
        fmt::join(unique_index.ptrs_, ", ")
    );
    auto l_stmt = sqlite_stmt(*this, l_create_unique_index_sql);
    l_stmt.step();
  }
}

storage::~storage() { ::sqlite3_close_v2(db_); }

storage& storage::finalize() {
  if (finalized_) return *this;
  finalized_ = true;
  for (auto& table : tables_) {
    for (auto& func : table->to_register_) {
      func(*this);
    }
  }

  return *this;
}
std::string storage::get_table_name(std::type_index in_type_index) const {
  if (!type_to_table_index_.contains(in_type_index)) {
    throw std::runtime_error("Table not found for the given type");
  }
  auto l_table_index = type_to_table_index_.at(in_type_index);
  return tables_[l_table_index]->name_;
}

}  // namespace orm

}  // namespace doodle