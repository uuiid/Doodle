#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/delete.h>

namespace doodle::orm {
std::string delete_t::to_sql(bool in_include_table_name) const {
  if (!wheres_) throw std::runtime_error("WHERE condition is required for DELETE operation");

  auto l_sql = fmt::format("DELETE FROM {} WHERE {}", from_table_name_, wheres_->to_sql(*s_, in_include_table_name));
  return l_sql;
}
delete_t& delete_t::operator()() {
  if (!stmt_) {
    auto l_sql = to_sql(false);
    stmt_      = std::make_shared<sqlite_stmt>();
    stmt_->prepare(*s_, l_sql);
    wheres_->collect_bind_variants(bind_variants_);
  }
  stmt_->reset_bind();
  for (const auto& val : bind_variants_.bind_values_) val.bind(*stmt_);
  return *this;
}
}  // namespace doodle::orm