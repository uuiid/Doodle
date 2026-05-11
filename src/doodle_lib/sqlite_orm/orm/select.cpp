#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {

std::string select_t::to_sql(const storage& s) const {
  std::string l_join_sql;
  for (const auto& join : joins_) {
    auto l_condition = join.on_condition_fun_(s);
    l_join_sql += fmt::format(
        " {} {} ON {} = {}", join.type_, s.get_table_name(join.join_table_type_index_), l_condition.first,
        l_condition.second
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
      "SELECT {} FROM {}{} {}{}{}", fmt::join(get_column_names_fun_(s), ", "), s.get_table_name(from_table_type_index_),
      l_join_sql, wheres_(s), l_order_by_sql, l_limit_sql
  );
  return l_sql;
}

}  // namespace doodle::orm