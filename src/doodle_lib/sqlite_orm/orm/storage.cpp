#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>
#include <doodle_lib/sqlite_orm/orm/storage_impl.h>

#include <fmt/format.h>
#include <string>
#include <vector>

namespace doodle {
namespace orm {

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

  std::string l_sql = fmt::format(
      "SELECT {} FROM {}{} {}", fmt::join(in_select.get_column_names_fun_(*this), ", "),
      get_table_name(in_select.from_table_type_index_), l_join_sql, in_select.wheres_.condition_fun_(*this)
  );

  return l_sql;
}

}  // namespace orm

}  // namespace doodle