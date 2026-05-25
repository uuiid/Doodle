#include <doodle_lib/sqlite_orm/orm/create_index.h>

#include <string>

namespace doodle::orm {
std::string create_index_base_t::to_sql(storage& s, to_sql_ctx ctx) const {
  std::vector<std::string> l_column_names_str{};
  for (const auto& column_name_ptr : info_->column_names_) {
    l_column_names_str.push_back(column_name_ptr->get_column_name(s, ctx));
  }
  if (info_->name_.empty())
    info_->name_ = fmt::format(
        "idx_{}_{}_{}", info_->table_name_->get_table_name(s), fmt::join(l_column_names_str, "_"),
        info_->unique_ ? "unique" : "index"
    );
  std::string l_create_index_sql = fmt::format(
      "CREATE {}INDEX IF NOT EXISTS {} ON {}({})", info_->unique_ ? "UNIQUE " : "", info_->name_,
      info_->table_name_->get_table_name(s), fmt::join(l_column_names_str, ", ")
  );
  if (info_->on_condition_) {
    l_create_index_sql += " WHERE " + info_->on_condition_->to_sql(s, ctx);
  }
  return l_create_index_sql;
}
}  // namespace doodle::orm