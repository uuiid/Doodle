#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/delete.h>

namespace doodle::orm {
delete_t& delete_t::operator()() {
  if (!wheres_) throw std::runtime_error("WHERE condition is required for DELETE operation");

  if (!stmt_) {
    auto l_sql = fmt::format("DELETE FROM {} WHERE {}", from_table_name_, wheres_->to_sql(*s_, false));
    stmt_      = std::make_shared<sqlite_stmt>();
    stmt_->prepare(*s_, l_sql);
    wheres_->collect_bind_variants(bind_variants_);
  }
  for (const auto& val : bind_variants_) stmt_->bind(*val);
  return *this;
}
}  // namespace doodle::orm