#pragma once

#include <doodle_lib/sqlite_orm/orm/fwd.h>

#include <string>
#include <utility>

namespace doodle::orm {
struct DOODLELIB_API table_info_t : public table_info_base_t {
  std::type_index type_index_{typeid(void)};
  explicit table_info_t(std::type_index in_type_index) : type_index_(in_type_index) {}
  // virtual std::string get_table_name(const session& s) const override;
  std::string to_sql(const session& s, const to_sql_ctx& ctx) const override;
  void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
};

// 直接持有表名的运行时表信息, 用于运行时才知道表名的场景
// (例如 PRAGMA foreign_key_check 返回的表名字符串, 无法在编译期映射到具体类型)
struct DOODLELIB_API table_name_info_t : public table_info_base_t {
  std::string table_name_;
  explicit table_name_info_t(std::string in_table_name) : table_name_(std::move(in_table_name)) {}
  std::string to_sql(const session&, const to_sql_ctx&) const override { return table_name_; }
  void collect_bind_variants(bind_value_collector_t&) const override {}
};

}  // namespace doodle::orm