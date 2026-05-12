#include <doodle_core/doodle_core_fwd.h>

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
  std::vector<std::string> l_order_by_clauses;
  l_order_by_clauses.reserve(order_bys_.size());
  for (const auto& order_by : order_bys_) {
    l_order_by_clauses.push_back(order_by(s));
  }
  std::string l_order_by_sql =
      l_order_by_clauses.empty() ? "" : fmt::format(" ORDER BY {}", fmt::join(l_order_by_clauses, ", "));
  std::string l_limit_sql = limit_ ? fmt::format(" LIMIT {}", *limit_) : "";
  l_limit_sql += offset_ ? fmt::format(" OFFSET {}", *offset_) : "";

  std::string l_sql = fmt::format(
      "SELECT {} FROM {}{} {}{}{}", fmt::join(column_names_, ", "), from_table_name_, l_join_sql,
      wheres_ ? fmt::format("WHERE {}", wheres_->to_sql(s)) : "", l_order_by_sql, l_limit_sql
  );
  return l_sql;
}

}  // namespace doodle::orm