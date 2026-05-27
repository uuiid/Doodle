#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/create_index.h>

#include <string>


namespace doodle::orm {
std::string create_index_base_t::to_sql(storage& s, const to_sql_ctx& ctx) const {
  std::vector<std::string> l_column_names_str{};
  for (const auto& column_name_ptr : info_->column_names_) {
    l_column_names_str.push_back(column_name_ptr->get_column_name(s, ctx));
  }
  if (info_->name_.empty())
    info_->name_ = fmt::format(
        "idx_{}_{}_{}", info_->table_name_->get_table_name(s), fmt::join(l_column_names_str, "_"),
        info_->unique_ ? "unique" : "index"
    );
  boost::algorithm::replace_all(info_->name_, "\"", "");
  std::string l_create_index_sql = fmt::format(
      "CREATE {}INDEX IF NOT EXISTS {} ON {}({})", info_->unique_ ? "UNIQUE " : "", info_->name_,
      info_->table_name_->get_table_name(s), fmt::join(l_column_names_str, ", ")
  );
  if (info_->on_condition_) {
    l_create_index_sql += " WHERE " + info_->on_condition_->to_sql(s, ctx);
  }
  return l_create_index_sql;
}

create_index_base_t::index_info create_index_base_t::get_index_info(storage& s, const to_sql_ctx& ctx) const {
  index_info info{};
  info.name_          = info_->name_;
  info.table_name_    = info_->table_name_->get_table_name(s);
  info.unique_        = info_->unique_;
  info.has_condition_ = static_cast<bool>(info_->on_condition_);
  for (const auto& column_name_ptr : info_->column_names_) {
    info.column_names_.push_back(column_name_ptr->get_column_name(s, ctx));
  }
  return info;
}

}  // namespace doodle::orm