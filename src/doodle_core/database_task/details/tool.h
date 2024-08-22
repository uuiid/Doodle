#pragma once

#include "doodle_core/database_task/sql_com.h"
#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/database_task/details/column.h>
#include <doodle_core/database_task/details/drop_if.h>
#include <doodle_core/database_task/details/tables.h>
#include <doodle_core/metadata/metadata.h>

#include "fmt/color.h"
#include "fmt/core.h"
#include "sqlpp11/custom_query.h"
#include "sqlpp11/select.h"
#include "sqlpp11/where.h"
#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <sqlpp11/data_types/boolean/data_type.h>
#include <sqlpp11/data_types/integral/data_type.h>
#include <sqlpp11/data_types/text/data_type.h>
#include <sqlpp11/data_types/time_point/data_type.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

namespace doodle::database_n::tables {
namespace sql_structure {
DOODLE_SQL_COLUMN_IMP(type, sqlpp::text, doodle::database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(name, sqlpp::text, doodle::database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(tbl_name, sqlpp::text, doodle::database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(rootpage, sqlpp::text, doodle::database_n::detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(sql, sqlpp::text, doodle::database_n::detail::can_be_null);
}  // namespace sql_structure

struct sqlite_master : sqlpp::table_t<
                           sqlite_master, sql_structure::type, sql_structure::name, sql_structure::tbl_name,
                           sql_structure::rootpage, sql_structure::sql> {
  struct _alias_t {
    static constexpr const char _literal[] = "sqlite_master";
    using _name_t                          = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
    template <typename t>
    struct _member_t {
      t sqlite_master;
      t& operator()() { return sqlite_master; }
      const t& operator()() const { return sqlite_master; }
    };
  };
};

}  // namespace doodle::database_n::tables

namespace doodle::database_n::detail {
struct pragma_table : sqlpp::table_t<pragma_table, tables::column::name> {
  struct _alias_t {
    static constexpr const char _literal[] = "pragma_table";
    using _name_t                          = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
    template <typename t>
    struct _member_t {
      t pragma_table;
      t& operator()() { return pragma_table; }
      const t& operator()() const { return pragma_table; }
    };
  };
};

template <typename table_t>
bool has_table(const table_t& /*table*/, const sql_connection_ptr& in_connection) {
  const tables::sqlite_master l_table{};
  for (auto&& row :
       (*in_connection)(sqlpp::select(sqlpp::count(l_table.name))
                            .from(l_table)
                            .where(
                                l_table.name == (sqlpp::name_of<table_t>::template char_ptr<create_table_ctx>()) &&
                                l_table.type == "table"
                            ))) {
    return row.count.value() > 0;
  }
  //  pragma_table l_tab{};
  //  SQLPP_ALIAS_PROVIDER(tables::column::name);
  //  sqlpp::custom_query(sqlpp::count(l_tab.name)), sqlpp::verbatim(fmt::format("pragma_table_info", "")),
  //      sqlpp::where(l_tab.name == sqlpp::parameter(l_tab.name));

  return false;
}
template <typename column_t>
bool has_colume2(const sql_connection_ptr& in_connection, const column_t& in_column) {
  pragma_table l_tab{};
  for (auto&& row : (*in_connection)(
           sqlpp::custom_query(
               sqlpp::select(sqlpp::count(l_tab.name)),
               sqlpp::verbatim(fmt::format(
                   "FROM pragma_table_info('{}') AS pragma_table",
                   sqlpp::name_of<decltype(in_column.table())>::template char_ptr<create_table_ctx>()
               )),
               sqlpp::where(l_tab.name == std::string{sqlpp::name_of<column_t>::template char_ptr<create_table_ctx>()})
           )
               .with_result_type_of(sqlpp::select(sqlpp::count(l_tab.name)))
       )) {
    return row.count.value() > 0;
  }
  return false;
}

template <typename table_t>
struct sql_create_table_base {
 private:
  template <typename table_sub_t>
  void impl_create_table_parent_id(const doodle::sql_connection_ptr& in_ptr) {
    const table_sub_t l_table{};
    in_ptr->execute(detail::create_table(l_table).foreign_column(l_table.parent_id, table_t{}.id).end());
    in_ptr->execute(detail::create_index(l_table.parent_id));
    in_ptr->execute(detail::create_index(l_table.id));
  };

 protected:
  template <typename... table_subs_t>
  void create_table_parent_id(const sql_connection_ptr& in_ptr) {
    (impl_create_table_parent_id<table_subs_t>(in_ptr), ...);
  }
  template <typename column_t>
  void create_table(const sql_connection_ptr& in_ptr, const column_t& in_column) {
    if (!has_colume2(in_ptr, in_column)) {
      in_ptr->execute(detail::create_colume(in_column));
    }
  }

  template <typename column_t>
  bool has_colume(const sql_connection_ptr& in_ptr, const column_t& in_column) {
    return has_colume2(in_ptr, in_column);
  }

 public:
  sql_create_table_base() = default;
  virtual void create_table(const sql_connection_ptr& in_ptr) {
    const table_t l_tables{};
    in_ptr->execute(detail::create_table(l_tables).foreign_column(l_tables.entity_id, tables::entity{}.id).end());
    in_ptr->execute(detail::create_index(l_tables.id));
    in_ptr->execute(detail::create_index(l_tables.entity_id));
  };

  bool has_table(const sql_connection_ptr& in_ptr) {
    const table_t l_tables{};
    return doodle::database_n::detail::has_table(l_tables, in_ptr);
  }

  std::tuple<std::map<std::int64_t, entt::handle>, std::vector<entt::handle>> split_update_install(
      const doodle::sql_connection_ptr& in_ptr, const std::vector<entt::handle>& in_entts
  ) {
    const table_t l_table{};

    auto l_pre_ = in_ptr->prepare(
        sqlpp::select(l_table.id).from(l_table).where(l_table.entity_id == sqlpp::parameter(l_table.entity_id))
    );
    std::map<std::int64_t, entt::handle> l_updata_map{};
    std::vector<entt::handle> l_install;
    for (auto&& e : in_entts) {
      l_pre_.params.entity_id = e.get<database>().get_id();
      bool has_{};
      for (auto&& i : (*in_ptr)(l_pre_)) {
        l_updata_map.emplace(i.id.value(), e);
        has_ = true;
      }
      if (!has_) {
        l_install.emplace_back(e);
      }
    }
    return {l_updata_map, l_install};
  }
};
template <>
struct sql_create_table_base<tables::entity> {
 public:
  sql_create_table_base() = default;
  virtual void create_table(const doodle::sql_connection_ptr& in_ptr) {
    const tables::entity l_tables{};
    in_ptr->execute(detail::create_table(l_tables).unique_column(l_tables.uuid_data).end());
    in_ptr->execute(detail::create_index(l_tables.id));
  };

  bool has_table(const doodle::sql_connection_ptr& in_ptr) {
    const tables::entity l_tables{};
    return doodle::database_n::detail::has_table(l_tables, in_ptr);
  }

  std::tuple<std::map<std::int64_t, entt::entity>, std::vector<entt::entity>> split_update_install(
      const doodle::sql_connection_ptr& in_ptr, const std::vector<entt::entity>& in_entts, const registry_ptr& reg_
  ) {
    const tables::entity l_table{};

    auto l_pre_ =
        in_ptr->prepare(sqlpp::select(l_table.id).from(l_table).where(l_table.id == sqlpp::parameter(l_table.id)));
    std::map<std::int64_t, entt::entity> l_updata_map{};
    std::vector<entt::entity> l_install;
    for (auto&& e : in_entts) {
      l_pre_.params.id = reg_->get<database>(e).get_id();
      bool has_{};
      for (auto&& i : (*in_ptr)(l_pre_)) {
        l_updata_map.emplace(i.id.value(), e);
        has_ = true;
      }
      if (!has_) {
        l_install.emplace_back(e);
      }
    }
    return {l_updata_map, l_install};
  }
};

}  // namespace doodle::database_n::detail