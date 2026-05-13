#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <fmt/format.h>

namespace doodle::orm {

std::string select_t::to_sql(const storage& s) const {
  std::string l_join_sql;
  for (const auto& join : joins_) {
    l_join_sql += fmt::format(
        " {} {} ON {} = {}", join.type_, join.join_table_name_, join.condition_.first, join.condition_.second
    );
  }

  std::string l_order_by_sql = order_bys_.empty() ? "" : fmt::format(" ORDER BY {}", fmt::join(order_bys_, ", "));
  std::string l_limit_sql    = limit_ ? fmt::format(" LIMIT {}", *limit_) : "";
  l_limit_sql += offset_ ? fmt::format(" OFFSET {}", *offset_) : "";

  std::string l_sql = fmt::format(
      "SELECT {} FROM {}{} {}{}{}", fmt::join(column_names_, ", "), from_table_name_, l_join_sql,
      wheres_ ? fmt::format("WHERE {}", wheres_->to_sql(s, true)) : "", l_order_by_sql, l_limit_sql
  );
  return l_sql;
}

void select_t::collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const {
  if (wheres_) {
    wheres_->collect_bind_variants(bind_variants);
  }
}
void select_t::run() {
  if (!stmt_) {
    auto l_sql = to_sql(*s_);
    stmt_      = std::make_shared<sqlite_stmt>();
    stmt_->prepare(*s_, l_sql);
    wheres_->collect_bind_variants(bind_variants_);
  }
  stmt_->reset_bind();
  for (const auto& val : bind_variants_) {
    stmt_->bind(*val);
  }
}
}  // namespace doodle::orm