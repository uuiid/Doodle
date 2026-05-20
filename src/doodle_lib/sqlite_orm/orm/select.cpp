#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include "fwd.h"
#include <fmt/base.h>
#include <fmt/format.h>
#include <string>
#include <vector>

namespace doodle::orm {

std::string select_t::to_sql(const storage& s) const {
  std::vector<std::string> l_column_names_str{};
  for (const auto& column_name_ptr : impl_->column_names_)
    l_column_names_str.push_back(column_name_ptr->get_column_name(s, true));

  std::string l_join_sql;
  for (const auto& join : impl_->joins_) {
    l_join_sql += fmt::format(
        " {} {} ON {} = {}", join.type_, join.join_table_info_->get_table_name(*impl_->s_),
        join.self_column_info_->get_column_name(*impl_->s_, true),
        join.join_column_info_->get_column_name(*impl_->s_, true)
    );
  }
  std::string l_group_by_sql{};
  {
    std::vector<std::string> l_group_by_column_names{};
    for (const auto& group_by_column : impl_->group_bys_)
      l_group_by_column_names.push_back(group_by_column->get_column_name(s, true));
    if (!l_group_by_column_names.empty())
      l_group_by_sql = fmt::format(" GROUP BY {}", fmt::join(l_group_by_column_names, ", "));
  }

  std::string l_order_by_sql =
      impl_->order_bys_.empty() ? "" : fmt::format(" ORDER BY {}", fmt::join(impl_->order_bys_, ", "));
  std::string l_limit_sql = impl_->limit_ ? fmt::format(" LIMIT {}", *impl_->limit_) : "";
  l_limit_sql += impl_->offset_ ? fmt::format(" OFFSET {}", *impl_->offset_) : "";

  std::string l_sql = fmt::format(
      "SELECT {col} FROM {tab} {join} {where} {group_by} {order_by} {limit}",
      fmt::arg("col", fmt::join(l_column_names_str, ", ")), fmt::arg("tab", impl_->from_table_name_),
      fmt::arg("join", l_join_sql),
      fmt::arg("where", impl_->wheres_ ? fmt::format("WHERE {}", impl_->wheres_->to_sql(s, true)) : ""),
      fmt::arg("group_by", l_group_by_sql), fmt::arg("order_by", l_order_by_sql), fmt::arg("limit", l_limit_sql)
  );
  return l_sql;
}

void select_t::collect_bind_variants(bind_value_collector_t& bind_variants) const {
  if (impl_->wheres_) {
    impl_->wheres_->collect_bind_variants(bind_variants);
  }
}
void select_t::run() {
  if (!impl_->stmt_) {
    auto l_sql   = to_sql(*impl_->s_);
    impl_->stmt_ = std::make_shared<sqlite_stmt>();
    impl_->stmt_->prepare(*impl_->s_, l_sql);
    if (impl_->wheres_) {
      impl_->wheres_->collect_bind_variants(impl_->bind_variants_);
    }
  }
  impl_->stmt_->reset_bind();
  for (const auto& val : impl_->bind_variants_.bind_values_) val.bind(*impl_->stmt_);
}
}  // namespace doodle::orm