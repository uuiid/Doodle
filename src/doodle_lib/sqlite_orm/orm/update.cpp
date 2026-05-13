#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>
#include <doodle_lib/sqlite_orm/orm/update.h>

#include <string>
#include <vector>

namespace doodle::orm {
update_t& update_t::operator()() & {
  if (!wheres_) throw std::runtime_error("WHERE condition is required for UPDATE operation");

  if (!stmt_) {
    std::vector<std::string> l_set_clauses;
    for (const auto& col_op : column_operations_) {
      l_set_clauses.push_back(col_op->to_sql(*s_, false));
    }

    auto l_sql = fmt::format(
        "UPDATE {} SET {} WHERE {}", from_table_name_, fmt::join(l_set_clauses, ", "), wheres_->to_sql(*s_, false)
    );
    stmt_ = std::make_shared<sqlite_stmt>();
    stmt_->prepare(*s_, l_sql);
    bind_variants_.clear();
    for (const auto& col_op : column_operations_) {
      col_op->collect_bind_variants(bind_variants_);
    }
    wheres_->collect_bind_variants(bind_variants_);
  }
  stmt_->reset_bind_index();
  for (const auto& val : bind_variants_) stmt_->bind(*val);

  stmt_->step();
  return *this;
}
}  // namespace doodle::orm