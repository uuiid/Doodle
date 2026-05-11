#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/exception.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>
#include <doodle_lib/sqlite_orm/orm/storage_impl.h>

#include <fmt/format.h>
#include <sqlite3.h>
#include <string>
#include <vector>

namespace doodle {
namespace orm {

std::int32_t sqlite_stmt::get_bind_index() {
  bind_index_++;
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
sqlite_stmt::~sqlite_stmt() { sqlite3_finalize(stmt_); }

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
    auto l_sql          = table->get_column_create_sql();
    auto l_foreign_keys = table->get_foreign_key_create_sql();
    l_sql.insert(l_sql.end(), l_foreign_keys.begin(), l_foreign_keys.end());
    auto l_create_table_sql = fmt::format("CREATE TABLE IF NOT EXISTS {} ({})", table->name_, fmt::join(l_sql, ", "));
    auto l_stmt             = sqlite_stmt(*this, l_create_table_sql);
    DOODLE_ORM_ERROR_SQLITE3(sqlite3_step(l_stmt.stmt_), db_);
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

std::string storage::compile_select(const select_t& in_select) const {
  std::string l_join_sql;
  for (const auto& join : in_select.joins_) {
    auto l_condition = join.on_condition_fun_(*this);
    l_join_sql += fmt::format(
        " {} {} ON {} = {}", join.type_, get_table_name(join.join_table_type_index_), l_condition.first,
        l_condition.second
    );
  }
  std::vector<std::string> l_order_by_clauses;
  l_order_by_clauses.reserve(in_select.order_bys_.size());
  for (const auto& order_by : in_select.order_bys_) {
    l_order_by_clauses.push_back(order_by(*this));
  }
  std::string l_order_by_sql =
      l_order_by_clauses.empty() ? "" : fmt::format(" ORDER BY {}", fmt::join(l_order_by_clauses, ", "));
  std::string l_limit_sql = in_select.limit_ ? fmt::format(" LIMIT {}", *in_select.limit_) : "";
  l_limit_sql += in_select.offset_ ? fmt::format(" OFFSET {}", *in_select.offset_) : "";

  std::string l_sql = fmt::format(
      "SELECT {} FROM {}{} {}{}{}", fmt::join(in_select.get_column_names_fun_(*this), ", "),
      get_table_name(in_select.from_table_type_index_), l_join_sql, in_select.wheres_.condition_fun_(*this),
      l_order_by_sql, l_limit_sql
  );
  return l_sql;
}

}  // namespace orm

}  // namespace doodle