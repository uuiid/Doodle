#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/alias.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/table_info.h>

namespace doodle::orm {
// 子选择别名表
struct subquery_alias_info_t : public table_info_base_t {
  std::string alias_name_;
  select_t subquery_;
  explicit subquery_alias_info_t(std::string in_alias_name, select_t in_subquery)
      : alias_name_(std::move(in_alias_name)), subquery_(std::move(in_subquery)) {}
  // std::string get_table_name(const storage& s) const override;
  std::string to_sql(const storage& s, const to_sql_ctx& ctx) const override;
  void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
  template <typename Table, typename ValueType>
  result_column_info_t<Table, ValueType, alias_column_info_t> operator->*(ValueType Table::* in_column_alias) {
    return result_column_info_t<Table, ValueType, alias_column_info_t>{in_column_alias, alias_name_};
  }
  template <typename Table>
  result_column_info_t<Table, void, alias_info_t> object() {
    return result_column_info_t<Table, void, alias_info_t>{alias_name_, typeid(Table)};
  }
};

inline subquery_alias_info_t alias(const std::string& alias_name, const select_t& subquery) {
  return subquery_alias_info_t{alias_name, subquery};
}
}  // namespace doodle::orm