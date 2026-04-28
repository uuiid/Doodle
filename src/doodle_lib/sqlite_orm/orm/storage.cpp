#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <string>
#include <vector>

namespace doodle {
namespace orm {

std::string select_info::to_sql(const storage& in_storage) const {
  // 根据 select_info 中的 table_indices_ 和 column_ptrs_ 构建 SQL 查询字符串
  std::vector<std::string> column_names;
  std::string table_name;
  std::vector<std::string> join_clauses;
  for (size_t i = 0; i < column_ptrs_.size(); ++i) {
    if (std::holds_alternative<std::type_index>(column_ptrs_[i])) {
      auto type_idx       = std::get<std::type_index>(column_ptrs_[i]);
      auto l_colume_index = in_storage.type_to_column_index_.at(type_idx);
      column_names.emplace_back(
          fmt::format(
              R"("{}"."{}")", in_storage.tables_[l_colume_index.first].name_,
              in_storage.tables_[l_colume_index.first].columns_[l_colume_index.second].name_
          )
      );
    } else if (std::holds_alternative<object_t>(column_ptrs_[i])) {
      auto obj           = std::get<object_t>(column_ptrs_[i]);
      auto l_table_index = in_storage.type_to_table_index_.at(obj.table_index_);
      for (const auto& column : in_storage.tables_[l_table_index].columns_) {
        column_names.emplace_back(
            fmt::format(R"("{}"."{}")", in_storage.tables_.at(l_table_index).name_, column.name_)
        );
      }
    }
  }
  table_name = in_storage.tables_.at(in_storage.type_to_table_index_.at(from_table_index_)).name_;
  for (const auto& join : joins_) {
    auto l_table_index = in_storage.type_to_table_index_.at(join.table_index_);
    auto l_left_index  = in_storage.type_to_column_index_.at(join.left_right_ptrs_.first);
    auto l_right_index = in_storage.type_to_column_index_.at(join.left_right_ptrs_.second);
    join_clauses.emplace_back(
        fmt::format(
            R"({} "{}" ON "{}"."{}" = "{}"."{}")", join.type_, in_storage.tables_.at(l_table_index).name_,
            in_storage.tables_.at(l_left_index.first).name_,
            in_storage.tables_.at(l_left_index.first).columns_[l_left_index.second].name_,
            in_storage.tables_.at(l_right_index.first).name_,
            in_storage.tables_.at(l_right_index.first).columns_[l_right_index.second].name_
        )
    );
  }

  std::string sql =
      fmt::format("SELECT {} FROM {} {}", fmt::join(column_names, ", "), table_name, fmt::join(join_clauses, " "));
  return sql;
}

}  // namespace orm

}  // namespace doodle