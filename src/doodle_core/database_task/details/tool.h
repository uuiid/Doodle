#pragma once

#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/metadata/metadata.h>

#include "fmt/format.h"
#include "sqlpp11/all_of.h"
#include "sqlpp11/type_traits.h"
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
namespace doodle::database_n::detail {
#include <string_view>

template <typename T>
void sql_com_destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  auto& l_conn = *in_ptr;
#include <string_view>
  T l_tabl{};
  auto l_pre = l_conn.prepare(sqlpp::remove_from(l_tabl).where(l_tabl.entity_id == sqlpp::parameter(l_tabl.entity_id)));

  for (auto& l_id : in_handle) {
    l_pre.params.entity_id = boost::numeric_cast<std::int64_t>(l_id);
    l_conn(l_pre);
  }
}

template <typename T>
void sql_com_destroy_parent_id(conn_ptr& in_ptr, const std::map<entt::handle, std::int64_t>& in_handle) {
  auto& l_conn = *in_ptr;
  const T l_tabl{};
  auto l_pre = l_conn.prepare(sqlpp::remove_from(l_tabl).where(l_tabl.parent_id == sqlpp::parameter(l_tabl.parent_id)));

  for (auto&& [l_k, l_id] : in_handle) {
    l_pre.params.parent_id = boost::numeric_cast<std::int64_t>(l_id);
    l_conn(l_pre);
  }
}
template <typename T>
void sql_com_destroy_parent_id(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  auto& l_conn = *in_ptr;
  const T l_tabl{};
  auto l_pre = l_conn.prepare(sqlpp::remove_from(l_tabl).where(l_tabl.parent_id == sqlpp::parameter(l_tabl.parent_id)));

  for (auto&& l_id : in_handle) {
    l_pre.params.parent_id = boost::numeric_cast<std::int64_t>(l_id);
    l_conn(l_pre);
  }
}
template <typename T, typename... sub_types>
auto sql_com_destroy_parent_id_return_id(conn_ptr& in_ptr, const std::vector<entt::handle>& in_handle) {
  auto& l_conn = *in_ptr;
  const T l_table{};
  std::map<entt::handle, std::int64_t> map_id{};
  auto l_pre_select = l_conn.prepare(
      sqlpp::select(l_table.id).from(l_table).where(l_table.entity_id == sqlpp::parameter(l_table.entity_id))
  );
  for (auto& l_h : in_handle) {
    l_pre_select.params.entity_id = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    for (auto&& row : l_conn(l_pre_select)) {
      map_id.emplace(l_h, row.id.value());
    }
  }
  ((sql_com_destroy_parent_id<sub_types>(in_ptr, map_id), void()), ...);
  return map_id;
}

template <typename T>
inline sqlpp::make_traits<T, sqlpp::tag::can_be_null> can_be_null();

template <typename T>
inline sqlpp::make_traits<T, sqlpp::tag::require_insert> require_insert();

template <typename table_t>
struct vector_database {
  template <typename vector_value_t>
  auto insert_prepare(conn_ptr& in_ptr) {
    auto& l_conn = *in_ptr;
    const table_t l_table{};

    return l_conn.prepare(sqlpp::insert_into(l_table).set(
        l_table.value = sqlpp::parameter(l_table.value), l_table.parent_id = sqlpp::parameter(l_table.parent_id)
    ));
  }
};

struct create_table_ctx {};

template <typename table_t>
struct create_table_t {
  std::string sql_data{};

 private:
  template <typename column_t, std::enable_if_t<sqlpp::is_integral_t<column_t>::value>* = nullptr>
  constexpr static std::string_view get_type_name(const column_t& /*in_column*/) {
    return "INTEGER";
  }
  template <typename column_t, std::enable_if_t<sqlpp::is_text_t<column_t>::value>* = nullptr>
  constexpr static std::string_view get_type_name(const column_t& /*in_column*/) {
    return "TEXT";
  }
  template <typename column_t, std::enable_if_t<sqlpp::is_time_point_t<column_t>::value>* = nullptr>
  constexpr static std::string_view get_type_name(const column_t& /*in_column*/) {
    return "DATETIME";
  }

  template <typename column_t>
  auto impl_primary_key_column(const column_t& in_column) {
    sql_data += fmt::format("{} {} PRIMARY KEY", sqlpp::name_of<column_t>::template char_ptr<create_table_ctx>());
    return *this;
  }
  template <typename column_t>
  auto impl_column(const column_t& /*in_column*/) {
    sql_data += fmt::format(", {} {}", sqlpp::name_of<column_t>::template char_ptr<create_table_ctx>());
    return *this;
  }

 public:
  explicit create_table_t() {
    sql_data =
        fmt::format("CREATE TABLE IF NOT EXISTS {} ( ", sqlpp::name_of<table_t>::template char_ptr<create_table_ctx>());
    table_t l_table{};
    std::apply([this](auto&&... x) { column(x...); }, sqlpp::all_of(l_table));
  }

  template <typename primary_column_t, typename... columns_t>
  auto column(const primary_column_t& in_primary_column, const columns_t&... in_columns) {
    impl_primary_key_column<primary_column_t>(in_primary_column);
    (impl_column<columns_t>(in_columns), ...);
    return *this;
  }

  template <typename self_column_t, typename foreign_column_t>
  auto foreign_column(const self_column_t& in_self_column, const foreign_column_t& in_foreign_column) {
    sql_data += fmt::format(
        R"(, FOREIGN KEY ({}) REFERENCES {} ({})  ON DELETE CASCADE ON UPDATE CASCADE)",
        sqlpp::name_of<self_column_t>::template char_ptr<create_table_ctx>(),
        sqlpp::name_of<decltype(in_foreign_column.table())>::template char_ptr<create_table_ctx>(),
        sqlpp::name_of<foreign_column_t>::template char_ptr<create_table_ctx>()
    );
    return *this;
  }

  operator std::string() const { return sql_data + ");"; }
};
template <typename column_t>
std::string create_index(const column_t& in_column) {
  return fmt::format(
      "CREATE INDEX IF NOT EXISTS {0}_ix_{1} ON {0} ({1})",
      sqlpp::name_of<decltype(in_column.table())>::template char_ptr<create_table_ctx>(),
      sqlpp::name_of<column_t>::template char_ptr<create_table_ctx>()
  );
}

template <typename table_t>
create_table_t<table_t> create_table(const table_t& in_table) {
  return create_table_t<table_t>{};
}

};  // namespace doodle::database_n::detail

#define DOODLE_SQL_COLUMN_IMP(column_name, type, tag)                                                 \
  struct column_name {                                                                                \
    struct _alias_t {                                                                                 \
      static constexpr const char _literal[] = #column_name;                                          \
      using _name_t                          = sqlpp::make_char_sequence<sizeof(_literal), _literal>; \
      template <typename T>                                                                           \
      struct _member_t {                                                                              \
        T column_name;                                                                                \
        T& operator()() { return column_name; }                                                       \
        const T& operator()() const { return column_name; }                                           \
      };                                                                                              \
    };                                                                                                \
    using _traits = decltype(tag<type>());                                                            \
  }

#define DOODLE_SQL_TABLE_IMP(table_name, ...)                                                                          \
  struct table_name : sqlpp::table_t<table_name, __VA_ARGS__> {                                                        \
    struct _alias_t {                                                                                                  \
      static constexpr const char _literal[] = #table_name;                                                            \
      using _name_t                          = sqlpp::make_char_sequence<sizeof(_literal), _literal>;                  \
      template <typename T>                                                                                            \
      struct _member_t {                                                                                               \
        T table_name;                                                                                                  \
        T& operator()() { return table_name; }                                                                         \
        const T& operator()() const { return table_name; }                                                             \
      };                                                                                                               \
    };                                                                                                                 \
  };                                                                                                                   \
  static_assert(                                                                                                       \
      std::is_same_v<std::tuple_element<0, decltype(sqlpp::all_of(table_name{}))>::type::_spec_t, tables::column::id>, \
      "ID is the primary key and must be placed first"                                                                 \
  )

namespace doodle::database_n::tables {
namespace column {
DOODLE_SQL_COLUMN_IMP(id, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(com_hash, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(json_data, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(uuid_data, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(update_time, sqlpp::time_point, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(entity_id, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(user_name, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(permission_group, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(user_id, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(task_name, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(region, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(abstract, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(time_point, sqlpp::time_point, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(eps, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(shot_int, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(shot_ab, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(redirection_path, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(redirection_file_name, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(assets_path, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(comment_string, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(comment_time, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(file_path, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(start_frame, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(end_frame, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(ref_file, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(export_type_, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(path, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(name, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(version, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(user_ref, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(first_time, sqlpp::time_point, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(second_time, sqlpp::time_point, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(info, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(is_extra_work, sqlpp::boolean, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(sim_path, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(export_group, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(cloth_proxy, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(simple_module_proxy, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(find_icon_regex, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(assets_list, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(icon_extensions, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(upload_path, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(season_count, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(use_only_sim_cloth, sqlpp::boolean, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(use_divide_group_export, sqlpp::boolean, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(use_rename_material, sqlpp::boolean, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(use_merge_mesh, sqlpp::boolean, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(t_post, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(export_anim_time, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(export_abc_arg, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(maya_camera_select_reg, sqlpp::text, detail::require_insert);
DOODLE_SQL_COLUMN_IMP(maya_camera_select_num, sqlpp::integer, detail::require_insert);
DOODLE_SQL_COLUMN_IMP(use_write_metadata, sqlpp::boolean, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(abc_export_extract_reference_name, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(abc_export_format_reference_name, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(abc_export_extract_scene_name, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(abc_export_format_scene_name, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(abc_export_add_frame_range, sqlpp::boolean, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(maya_camera_suffix, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(maya_out_put_abc_suffix, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(p_int, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(cutoff_p, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(p_name, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(p_path, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(p_en_str, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(p_shor_str, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(parent_id, sqlpp::integer, detail::can_be_null);

}  // namespace column
DOODLE_SQL_TABLE_IMP(entity, column::id, column::uuid_data, column::update_time);
DOODLE_SQL_TABLE_IMP(com_entity, column::id, column::entity_id, column::com_hash, column::json_data);

DOODLE_SQL_TABLE_IMP(usertab, column::id, column::entity_id, column::user_name, column::permission_group);
DOODLE_SQL_TABLE_IMP(
    work_task_info, column::id, column::entity_id, column::user_id, column::task_name, column::region, column::abstract,
    column::time_point
);
DOODLE_SQL_TABLE_IMP(episodes, column::id, column::entity_id, column::eps);
DOODLE_SQL_TABLE_IMP(shot, column::id, column::entity_id, column::shot_int, column::shot_ab);
DOODLE_SQL_TABLE_IMP(redirection_path_info, column::id, column::entity_id, column::redirection_file_name);
DOODLE_SQL_TABLE_IMP(rpi_search_path, column::id, column::parent_id, column::redirection_path);
DOODLE_SQL_TABLE_IMP(assets, column::id, column::entity_id, column::assets_path);
DOODLE_SQL_TABLE_IMP(comment, column::id, column::entity_id, column::comment_string, column::comment_time);
DOODLE_SQL_TABLE_IMP(
    export_file_info, column::id, column::entity_id, column::file_path, column::start_frame, column::end_frame,
    column::ref_file, column::export_type_
);
DOODLE_SQL_TABLE_IMP(image_icon, column::id, column::entity_id, column::path);
DOODLE_SQL_TABLE_IMP(
    assets_file, column::id, column::entity_id, column::name, column::path, column::version, column::user_ref
);
DOODLE_SQL_TABLE_IMP(
    time_point_info, column::id, column::entity_id, column::first_time, column::second_time, column::info,
    column::is_extra_work
);
DOODLE_SQL_TABLE_IMP(
    project_config, column::id, column::entity_id, column::sim_path, column::export_group, column::cloth_proxy,
    column::simple_module_proxy, column::find_icon_regex, column::upload_path, column::season_count,
    column::use_only_sim_cloth, column::use_divide_group_export, column::use_rename_material, column::use_merge_mesh,
    column::t_post, column::export_anim_time, column::export_abc_arg, column::use_write_metadata,
    column::abc_export_extract_reference_name, column::abc_export_format_reference_name,
    column::abc_export_extract_scene_name, column::abc_export_format_scene_name, column::abc_export_add_frame_range,
    column::maya_camera_suffix, column::maya_out_put_abc_suffix
);
DOODLE_SQL_TABLE_IMP(project_config_assets_list, column::id, column::parent_id, column::assets_list);
DOODLE_SQL_TABLE_IMP(project_config_icon_extensions, column::id, column::parent_id, column::icon_extensions);
DOODLE_SQL_TABLE_IMP(
    project_config_maya_camera_select, column::id, column::parent_id, column::maya_camera_select_num,
    column::maya_camera_select_reg
);

DOODLE_SQL_TABLE_IMP(season, column::id, column::entity_id, column::p_int);
DOODLE_SQL_TABLE_IMP(importance, column::id, column::entity_id, column::cutoff_p);
DOODLE_SQL_TABLE_IMP(
    project, column::id, column::entity_id, column::p_name, column::p_path, column::p_en_str, column::p_shor_str
);

namespace column {
DOODLE_SQL_COLUMN_IMP(work_weekdays, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(first_time_seconds, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(second_time_seconds, sqlpp::integer, detail::can_be_null);

DOODLE_SQL_COLUMN_IMP(work_index, sqlpp::integer, detail::can_be_null);

}  // namespace column

DOODLE_SQL_TABLE_IMP(business_rules, column::id, column::entity_id, column::work_weekdays);
DOODLE_SQL_TABLE_IMP(
    business_rules_work_pair, column::id, column::parent_id, column::first_time_seconds, column::second_time_seconds
);
DOODLE_SQL_TABLE_IMP(
    business_rules_work_abs_pair, column::id, column::work_index, column::parent_id, column::first_time_seconds,
    column::second_time_seconds
);
DOODLE_SQL_TABLE_IMP(
    business_rules_time_info_time_info, column::id, column::parent_id, column::first_time, column::second_time,
    column::info, column::is_extra_work
);

DOODLE_SQL_TABLE_IMP(time_point_wrap, column::id, column::entity_id, column::time_point);
}  // namespace doodle::database_n::tables
