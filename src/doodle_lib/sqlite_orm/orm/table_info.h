#pragma once

#include <doodle_lib/sqlite_orm/orm/fwd.h>

namespace doodle::orm {
struct table_info_t : public table_info_base_t {
  std::type_index type_index_{typeid(void)};
  explicit table_info_t(std::type_index in_type_index) : type_index_(in_type_index) {}
  // virtual std::string get_table_name(const session& s) const override;
  std::string to_sql(const session& s, const to_sql_ctx& ctx) const override;
  void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
};

}  // namespace doodle::orm