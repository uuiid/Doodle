#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/create_trigger.h>
#include <doodle_lib/sqlite_orm/orm/exception.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/sqlite_statement.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <fmt/format.h>
#include <sqlite3.h>
#include <string>
#include <vector>

namespace doodle {
namespace orm {

on_delete::on_delete(foreign_key_action action) : action_(action) {}

on_update::on_update(foreign_key_action action) : action_(action) {}

std::vector<std::string> table_info_base::get_foreign_key_create_sql() const {
  std::vector<std::string> l_sqls;
  for (const auto& fk : foreign_keys_) {
    std::string l_sql = fmt::format(
        "FOREIGN KEY({}) REFERENCES {}({}) ON DELETE {} ON UPDATE {}", fk.ptr_, fk.ref_table_, fk.ref_ptr_,
        fk.on_delete_, fk.on_update_
    );
    l_sqls.push_back(std::move(l_sql));
  }
  return l_sqls;
}

sqlite_stmt::sqlite_stmt(storage& db, const std::string& sql) { prepare(db, sql); }

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
storage::storage(FSys::path in_path, std::int32_t in_flags) { open(std::move(in_path), in_flags); }

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

void storage::open() { open({}); }

create_trigger_t storage::create_trigger(std::string in_name) {
  auto l_trigger_info = std::make_shared<trigger_info>();
  auto& l_trigger     = triggers_.emplace_back(l_trigger_info);
  return create_trigger_t{std::move(in_name), l_trigger_info, this};
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
  for (const auto& l_trigger : triggers_) {
    auto l_create_trigger_sql = fmt::format(
        "CREATE TRIGGER IF NOT EXISTS {} {} {} {} ON {} BEGIN {}; END;", l_trigger->name_, l_trigger->timing_,
        l_trigger->event_, fmt::format("{}", fmt::join(l_trigger->columns_, ", ")), l_trigger->table_name_,
        l_trigger->statement_
    );
    auto l_stmt = sqlite_stmt(*this, l_create_trigger_sql);
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