#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/insert.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <fmt/format.h>

namespace doodle::orm {

std::string insert_t::to_sql(bool in_include_table_name) const {
  auto l_values = fmt::format("({})", fmt::join(std::vector<std::string>(columns_.size(), "?"), ", "));
  if (batch_size_ > 1) l_values = fmt::format("{}", fmt::join(std::vector<std::string>(batch_size_, l_values), ", "));
  std::vector<std::string> l_column_names{};
  for (const auto& col_info_ptr : columns_) {
    l_column_names.push_back(col_info_ptr->get_column_name(*s_, in_include_table_name));
  }
  auto l_sql =
      fmt::format("INSERT INTO {} ({}) VALUES {}", into_table_name_, fmt::join(l_column_names, ", "), l_values);
  return l_sql;
}

insert_t& insert_t::operator()() {
  if (!stmt_) {
    auto l_sql = to_sql(false);
    stmt_      = std::make_shared<sqlite_stmt>();
    stmt_->prepare(*s_, l_sql);
  }
  stmt_->reset_bind();
  for (const auto& val : values_.bind_values_) val.bind(*stmt_);
  stmt_->step();
  return *this;
}

}  // namespace doodle::orm