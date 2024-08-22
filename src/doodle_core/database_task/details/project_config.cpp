#include "project_config.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/logger/logger.h>

#include "boost/numeric/conversion/cast.hpp"
#include <boost/winapi/error_codes.hpp>

#include "metadata/metadata.h"
#include "metadata/project.h"
#include "sqlpp11/custom_query.h"
#include "sqlpp11/select.h"
#include "sqlpp11/verbatim_table.h"
#include <algorithm>
#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <lib_warp/enum_template_tool.h>
#include <magic_enum.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>
#include <sqlpp11/aggregate_functions/count.h>
#include <sqlpp11/insert.h>
#include <sqlpp11/parameter.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/update.h>
#include <string>
#include <utility>
#include <vector>

namespace doodle::database_n {
void sql_com<project_config::base_config>::create_table(const sql_connection_ptr& in_ptr) {
  detail::sql_create_table_base<tables::project_config>::create_table(in_ptr);

  create_table_parent_id<
      tables::project_config_assets_list, tables::project_config_icon_extensions,
      tables::project_config_maya_camera_select>(in_ptr);
}
void sql_com<project_config::base_config>::install_sub(
    const sql_connection_ptr& in_ptr, const std::vector<entt::handle>& in_handles,
    const std::map<entt::handle, std::int64_t>& in_map
) {
  auto& l_conn = *in_ptr;
  {
    const tables::project_config_assets_list l_table{};
    auto l_path_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
        l_table.parent_id   = sqlpp::parameter(l_table.parent_id),
        l_table.assets_list = sqlpp::parameter(l_table.assets_list)
    ));
    for (auto& l_h : in_handles) {
      auto& l_r_p_i = l_h.get<doodle::project_config::base_config>();
      for (auto& l_p : l_r_p_i.assets_list) {
        l_path_pre.params.parent_id   = in_map.at(l_h);
        l_path_pre.params.assets_list = l_p;
        l_conn(l_path_pre);
      }
      //      BOOST_ASSERT(l_r_p == 1);
    }
  }

  {
    const tables::project_config_icon_extensions l_table{};
    auto l_path_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
        l_table.parent_id       = sqlpp::parameter(l_table.parent_id),
        l_table.icon_extensions = sqlpp::parameter(l_table.icon_extensions)
    ));
    for (auto& l_h : in_handles) {
      auto& l_r_p_i = l_h.get<doodle::project_config::base_config>();
      for (auto& l_p : l_r_p_i.icon_extensions) {
        l_path_pre.params.parent_id       = in_map.at(l_h);
        l_path_pre.params.icon_extensions = l_p;
        l_conn(l_path_pre);
      }
      //      BOOST_ASSERT(l_r_p == 1);
    }
  }
  {
    const tables::project_config_maya_camera_select l_table{};
    auto l_path_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
        l_table.parent_id              = sqlpp::parameter(l_table.parent_id),
        l_table.maya_camera_select_reg = sqlpp::parameter(l_table.maya_camera_select_reg),
        l_table.maya_camera_select_num = sqlpp::parameter(l_table.maya_camera_select_num)
    ));
    for (auto& l_h : in_handles) {
      auto& l_r_p_i = l_h.get<doodle::project_config::base_config>();
      for (auto&& [l_k, l_v] : l_r_p_i.maya_camera_select) {
        l_path_pre.params.parent_id              = in_map.at(l_h);
        l_path_pre.params.maya_camera_select_reg = l_k;
        l_path_pre.params.maya_camera_select_num = l_v;
        l_conn(l_path_pre);
        //        BOOST_ASSERT(l_r_p == 1);
      }
    }
  }
}

void sql_com<project_config::base_config>::insert(
    const sql_connection_ptr& in_ptr, const std::vector<entt::handle>& in_id
) {
  auto& l_conn = *in_ptr;

  std::map<entt::handle, std::int64_t> map_id{};

  {
    const tables::project_config l_table{};
    auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
        l_table.sim_path                          = sqlpp::parameter(l_table.sim_path),
        l_table.export_group                      = sqlpp::parameter(l_table.export_group),
        l_table.cloth_proxy                       = sqlpp::parameter(l_table.cloth_proxy),
        l_table.simple_module_proxy               = sqlpp::parameter(l_table.simple_module_proxy),
        l_table.find_icon_regex                   = sqlpp::parameter(l_table.find_icon_regex),
        l_table.upload_path                       = sqlpp::parameter(l_table.upload_path),
        l_table.season_count                      = sqlpp::parameter(l_table.season_count),
        l_table.use_only_sim_cloth                = sqlpp::parameter(l_table.use_only_sim_cloth),
        l_table.use_divide_group_export           = sqlpp::parameter(l_table.use_divide_group_export),
        l_table.t_post                            = sqlpp::parameter(l_table.t_post),
        l_table.export_anim_time                  = sqlpp::parameter(l_table.export_anim_time),
        l_table.abc_export_extract_reference_name = sqlpp::parameter(l_table.abc_export_extract_reference_name),
        l_table.abc_export_format_reference_name  = sqlpp::parameter(l_table.abc_export_format_reference_name),
        l_table.abc_export_extract_scene_name     = sqlpp::parameter(l_table.abc_export_extract_scene_name),
        l_table.abc_export_format_scene_name      = sqlpp::parameter(l_table.abc_export_format_scene_name),
        l_table.abc_export_add_frame_range        = sqlpp::parameter(l_table.abc_export_add_frame_range),
        l_table.maya_camera_suffix                = sqlpp::parameter(l_table.maya_camera_suffix),
        l_table.maya_out_put_abc_suffix           = sqlpp::parameter(l_table.maya_out_put_abc_suffix),
        l_table.entity_id                         = sqlpp::parameter(l_table.entity_id)
    ));

    for (auto& l_h : in_id) {
      auto& l_pconfig                                = l_h.get<project_config::base_config>();
      l_pre.params.sim_path                          = l_pconfig.vfx_cloth_sim_path.string();
      l_pre.params.export_group                      = l_pconfig.export_group;
      l_pre.params.cloth_proxy                       = l_pconfig.cloth_proxy_;
      l_pre.params.simple_module_proxy               = l_pconfig.simple_module_proxy_;
      l_pre.params.find_icon_regex                   = l_pconfig.find_icon_regex;
      l_pre.params.upload_path                       = l_pconfig.upload_path.string();
      l_pre.params.season_count                      = l_pconfig.season_count;
      l_pre.params.use_only_sim_cloth                = l_pconfig.use_only_sim_cloth;
      l_pre.params.use_divide_group_export           = l_pconfig.use_divide_group_export;
      l_pre.params.t_post                            = l_pconfig.t_post;
      l_pre.params.export_anim_time                  = l_pconfig.export_anim_time;
      l_pre.params.abc_export_extract_reference_name = l_pconfig.abc_export_extract_reference_name;
      l_pre.params.abc_export_format_reference_name  = l_pconfig.abc_export_format_reference_name;
      l_pre.params.abc_export_extract_scene_name     = l_pconfig.abc_export_extract_scene_name;
      l_pre.params.abc_export_format_scene_name      = l_pconfig.abc_export_format_scene_name;
      l_pre.params.abc_export_add_frame_range        = l_pconfig.abc_export_add_frame_range;
      l_pre.params.maya_camera_suffix                = l_pconfig.maya_camera_suffix;
      l_pre.params.maya_out_put_abc_suffix           = l_pconfig.maya_out_put_abc_suffix;

      l_pre.params.entity_id                         = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
      auto l_r                                       = l_conn(l_pre);
      map_id.emplace(l_h, l_r);
      //      DOODLE_LOG_INFO(
      //          "插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(),
      //          entt::type_id<project_config::base_config>().name()
      //      );
    }
  }
  install_sub(in_ptr, in_id, map_id);
}

void sql_com<project_config::base_config>::update(
    const sql_connection_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id
) {
  auto& l_conn = *in_ptr;

  {
    const tables::project_config l_table{};

    auto l_pre = l_conn.prepare(
        sqlpp::update(l_table)
            .set(
                l_table.sim_path                          = sqlpp::parameter(l_table.sim_path),
                l_table.export_group                      = sqlpp::parameter(l_table.export_group),
                l_table.cloth_proxy                       = sqlpp::parameter(l_table.cloth_proxy),
                l_table.simple_module_proxy               = sqlpp::parameter(l_table.simple_module_proxy),
                l_table.find_icon_regex                   = sqlpp::parameter(l_table.find_icon_regex),
                l_table.upload_path                       = sqlpp::parameter(l_table.upload_path),
                l_table.season_count                      = sqlpp::parameter(l_table.season_count),
                l_table.use_only_sim_cloth                = sqlpp::parameter(l_table.use_only_sim_cloth),
                l_table.use_divide_group_export           = sqlpp::parameter(l_table.use_divide_group_export),
                l_table.t_post                            = sqlpp::parameter(l_table.t_post),
                l_table.export_anim_time                  = sqlpp::parameter(l_table.export_anim_time),
                l_table.abc_export_extract_reference_name = sqlpp::parameter(l_table.abc_export_extract_reference_name),
                l_table.abc_export_format_reference_name  = sqlpp::parameter(l_table.abc_export_format_reference_name),
                l_table.abc_export_extract_scene_name     = sqlpp::parameter(l_table.abc_export_extract_scene_name),
                l_table.abc_export_format_scene_name      = sqlpp::parameter(l_table.abc_export_format_scene_name),
                l_table.abc_export_add_frame_range        = sqlpp::parameter(l_table.abc_export_add_frame_range),
                l_table.maya_camera_suffix                = sqlpp::parameter(l_table.maya_camera_suffix),
                l_table.maya_out_put_abc_suffix           = sqlpp::parameter(l_table.maya_out_put_abc_suffix)
            )
            .where(
                l_table.entity_id == sqlpp::parameter(l_table.entity_id) && l_table.id == sqlpp::parameter(l_table.id)
            )
    );
    for (auto& [id, l_h] : in_id) {
      auto& l_pconfig                                = l_h.get<project_config::base_config>();
      l_pre.params.sim_path                          = l_pconfig.vfx_cloth_sim_path.string();
      l_pre.params.export_group                      = l_pconfig.export_group;
      l_pre.params.cloth_proxy                       = l_pconfig.cloth_proxy_;
      l_pre.params.simple_module_proxy               = l_pconfig.simple_module_proxy_;
      l_pre.params.find_icon_regex                   = l_pconfig.find_icon_regex;
      l_pre.params.upload_path                       = l_pconfig.upload_path.string();
      l_pre.params.season_count                      = l_pconfig.season_count;
      l_pre.params.use_only_sim_cloth                = l_pconfig.use_only_sim_cloth;
      l_pre.params.use_divide_group_export           = l_pconfig.use_divide_group_export;
      l_pre.params.t_post                            = l_pconfig.t_post;
      l_pre.params.export_anim_time                  = l_pconfig.export_anim_time;
      l_pre.params.abc_export_extract_reference_name = l_pconfig.abc_export_extract_reference_name;
      l_pre.params.abc_export_format_reference_name  = l_pconfig.abc_export_format_reference_name;
      l_pre.params.abc_export_extract_scene_name     = l_pconfig.abc_export_extract_scene_name;
      l_pre.params.abc_export_format_scene_name      = l_pconfig.abc_export_format_scene_name;
      l_pre.params.abc_export_add_frame_range        = l_pconfig.abc_export_add_frame_range;
      l_pre.params.maya_camera_suffix                = l_pconfig.maya_camera_suffix;
      l_pre.params.maya_out_put_abc_suffix           = l_pconfig.maya_out_put_abc_suffix;

      l_pre.params.entity_id                         = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
      l_pre.params.id                                = id;
      auto l_r                                       = l_conn(l_pre);

      //      DOODLE_LOG_INFO(
      //          "更新数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(),
      //          entt::type_id<project_config::base_config>().name()
      //      );
    }
  }

  auto l_handles =
      in_id | ranges::views::transform([](auto&& l_pair) -> entt::handle { return l_pair.second; }) | ranges::to_vector;
  auto map_id = in_id | ranges::views::transform([](auto&& l_pair) -> std::pair<entt::handle, std::int64_t> {
                  return {l_pair.second, l_pair.first};
                }) |
                ranges::to<std::map<entt::handle, std::int64_t>>;
  detail::sql_com_destroy_parent_id<tables::project_config_assets_list>(in_ptr, map_id);
  detail::sql_com_destroy_parent_id<tables::project_config_icon_extensions>(in_ptr, map_id);
  detail::sql_com_destroy_parent_id<tables::project_config_maya_camera_select>(in_ptr, map_id);

  install_sub(in_ptr, l_handles, map_id);
}

void sql_com<project_config::base_config>::select(
    const sql_connection_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle, entt::registry& in_reg
) {
  auto& l_conn = *in_ptr;
  std::vector<project_config::base_config> l_config;
  std::vector<entt::entity> l_entts;
  std::map<entt::entity, std::int64_t> path_map{};
  {
    tables::project_config l_table{};
    // 调整内存
    for (auto&& raw :
         l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
      l_config.reserve(raw.count.value());
      l_entts.reserve(raw.count.value());
      break;
    }

    for (auto& row : l_conn(sqlpp::select(
                                l_table.id, l_table.sim_path, l_table.export_group, l_table.cloth_proxy,
                                l_table.simple_module_proxy, l_table.find_icon_regex, l_table.upload_path,
                                l_table.season_count, l_table.use_only_sim_cloth, l_table.use_divide_group_export,
                                l_table.t_post, l_table.export_anim_time, l_table.abc_export_extract_reference_name,
                                l_table.abc_export_format_reference_name, l_table.abc_export_extract_scene_name,
                                l_table.abc_export_format_scene_name, l_table.abc_export_add_frame_range,
                                l_table.maya_camera_suffix, l_table.maya_out_put_abc_suffix, l_table.entity_id
         )
                                .from(l_table)
                                .where(l_table.entity_id.is_not_null()))) {
      project_config::base_config l_p_c{};
      l_p_c.vfx_cloth_sim_path                = row.sim_path.value();
      l_p_c.export_group                      = row.export_group.value();
      l_p_c.cloth_proxy_                      = row.cloth_proxy.value();
      l_p_c.simple_module_proxy_              = row.simple_module_proxy.value();
      l_p_c.find_icon_regex                   = row.find_icon_regex.value();
      l_p_c.upload_path                       = row.upload_path.value();
      l_p_c.season_count                      = row.season_count.value();
      l_p_c.use_only_sim_cloth                = row.use_only_sim_cloth.value();
      l_p_c.use_divide_group_export           = row.use_divide_group_export.value();
      l_p_c.t_post                            = row.t_post.value();
      l_p_c.export_anim_time                  = row.export_anim_time.value();
      l_p_c.abc_export_extract_reference_name = row.abc_export_extract_reference_name.value();
      l_p_c.abc_export_format_reference_name  = row.abc_export_format_reference_name.value();
      l_p_c.abc_export_extract_scene_name     = row.abc_export_extract_scene_name.value();
      l_p_c.abc_export_format_scene_name      = row.abc_export_format_scene_name.value();
      l_p_c.abc_export_add_frame_range        = row.abc_export_add_frame_range.value();
      l_p_c.maya_camera_suffix                = row.maya_camera_suffix.value();
      l_p_c.maya_out_put_abc_suffix           = row.maya_out_put_abc_suffix.value();
      auto l_id                               = row.entity_id.value();
      if (in_handle.find(l_id) != in_handle.end()) {
        l_config.emplace_back(std::move(l_p_c));
        l_entts.emplace_back(in_handle.at(l_id));
        path_map.emplace(in_handle.at(l_id), row.id);
        // DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
      } else {
        // DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
      }
    }
  }

  {
    const tables::project_config_assets_list l_table{};
    auto l_pre = l_conn.prepare(
        sqlpp::select(l_table.assets_list).from(l_table).where(l_table.parent_id == sqlpp::parameter(l_table.parent_id))
    );
    for (auto i = 0; i < l_config.size(); ++i) {
      l_pre.params.parent_id = path_map[l_entts[i]];
      for (auto&& row : l_conn(l_pre)) {
        l_config[i].assets_list.emplace_back(row.assets_list.value());
      }
    }
  }

  {
    const tables::project_config_icon_extensions l_table{};
    auto l_pre = l_conn.prepare(sqlpp::select(l_table.icon_extensions)
                                    .from(l_table)
                                    .where(l_table.parent_id == sqlpp::parameter(l_table.parent_id)));
    for (auto i = 0; i < l_config.size(); ++i) {
      l_pre.params.parent_id = path_map[l_entts[i]];
      for (auto&& row : l_conn(l_pre)) {
        l_config[i].icon_extensions.emplace_back(row.icon_extensions.value());
      }
    }
  }
  {
    const tables::project_config_maya_camera_select l_table{};
    auto l_pre = l_conn.prepare(sqlpp::select(l_table.maya_camera_select_reg, l_table.maya_camera_select_num)
                                    .from(l_table)
                                    .where(l_table.parent_id == sqlpp::parameter(l_table.parent_id)));
    for (auto i = 0; i < l_config.size(); ++i) {
      l_pre.params.parent_id = path_map[l_entts[i]];
      for (auto&& row : l_conn(l_pre)) {
        l_config[i].maya_camera_select.emplace_back(
            row.maya_camera_select_reg.value(), boost::numeric_cast<std::int32_t>(row.maya_camera_select_num.value())
        );
      }
    }
  }

  in_reg.insert<project_config::base_config>(l_entts.begin(), l_entts.end(), l_config.begin());
}
void sql_com<project_config::base_config>::destroy(
    const sql_connection_ptr& in_ptr, const std::vector<std::int64_t>& in_handle
) {
  detail::sql_com_destroy<tables::project_config>(in_ptr, in_handle);
  detail::sql_com_destroy_parent_id<tables::project_config_assets_list>(in_ptr, in_handle);
  detail::sql_com_destroy_parent_id<tables::project_config_icon_extensions>(in_ptr, in_handle);
  detail::sql_com_destroy_parent_id<tables::project_config_maya_camera_select>(in_ptr, in_handle);
}

void sql_ctx<project_config::base_config>::create_table(const sql_connection_ptr& in_ptr) {
  detail::sql_create_table_base<tables::project_config>::create_table(in_ptr);

  create_table_parent_id<
      tables::project_config_assets_list, tables::project_config_icon_extensions,
      tables::project_config_maya_camera_select>(in_ptr);
}

void sql_ctx<project_config::base_config>::install_sub(
    const sql_connection_ptr& in_ptr, const std::int64_t in_id, const project_config::base_config& in_config
) {
  auto& l_conn = *in_ptr;
  {
    const tables::project_config_assets_list l_table{};

    auto l_path_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
        l_table.parent_id   = sqlpp::parameter(l_table.parent_id),
        l_table.assets_list = sqlpp::parameter(l_table.assets_list)
    ));

    for (auto& l_p : in_config.assets_list) {
      l_path_pre.params.parent_id   = in_id;
      l_path_pre.params.assets_list = l_p;
      l_conn(l_path_pre);
    }
  }

  {
    const tables::project_config_icon_extensions l_table{};
    auto l_path_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
        l_table.parent_id       = sqlpp::parameter(l_table.parent_id),
        l_table.icon_extensions = sqlpp::parameter(l_table.icon_extensions)
    ));
    for (auto& l_p : in_config.icon_extensions) {
      l_path_pre.params.parent_id       = in_id;
      l_path_pre.params.icon_extensions = l_p;
      l_conn(l_path_pre);
    }
  }
  {
    const tables::project_config_maya_camera_select l_table{};
    auto l_path_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
        l_table.parent_id              = sqlpp::parameter(l_table.parent_id),
        l_table.maya_camera_select_reg = sqlpp::parameter(l_table.maya_camera_select_reg),
        l_table.maya_camera_select_num = sqlpp::parameter(l_table.maya_camera_select_num)
    ));
    for (auto&& [l_k, l_v] : in_config.maya_camera_select) {
      l_path_pre.params.parent_id              = in_id;
      l_path_pre.params.maya_camera_select_reg = l_k;
      l_path_pre.params.maya_camera_select_num = l_v;
      l_conn(l_path_pre);
    }
  }
}

void sql_ctx<project_config::base_config>::insert(
    const sql_connection_ptr& in_ptr, const project_config::base_config& in_config
) {
  auto& l_conn = *in_ptr;

  tables::project_config const l_tabl{};
  auto l_select = l_conn(sqlpp::select(l_tabl.id).from(l_tabl).unconditionally());
  auto l_id     = l_select.empty() ? 0 : l_select.front().id.value();

  l_conn(sqlpp::sqlite3::insert_or_replace_into(l_tabl).set(
      l_tabl.id = l_id, l_tabl.sim_path = in_config.vfx_cloth_sim_path.string(),
      l_tabl.export_group = in_config.export_group, l_tabl.cloth_proxy = in_config.cloth_proxy_,
      l_tabl.simple_module_proxy = in_config.simple_module_proxy_, l_tabl.find_icon_regex = in_config.find_icon_regex,
      l_tabl.upload_path = in_config.upload_path.string(), l_tabl.season_count = in_config.season_count,
      l_tabl.use_only_sim_cloth      = in_config.use_only_sim_cloth,
      l_tabl.use_divide_group_export = in_config.use_divide_group_export, l_tabl.t_post = in_config.t_post,
      l_tabl.export_anim_time                  = in_config.export_anim_time,
      l_tabl.abc_export_extract_reference_name = in_config.abc_export_extract_reference_name,
      l_tabl.abc_export_format_reference_name  = in_config.abc_export_format_reference_name,
      l_tabl.abc_export_extract_scene_name     = in_config.abc_export_extract_scene_name,
      l_tabl.abc_export_format_scene_name      = in_config.abc_export_format_scene_name,
      l_tabl.abc_export_add_frame_range        = in_config.abc_export_add_frame_range,
      l_tabl.maya_camera_suffix                = in_config.maya_camera_suffix,
      l_tabl.maya_out_put_abc_suffix           = in_config.maya_out_put_abc_suffix
  ));
  //  auto l_main_id =
  //      l_conn(sqlpp::select(l_tabl.id).from(l_tabl).where(l_tabl.entity_id == l_entt_id)).front().id.value();
  std::vector<std::int64_t> l_main_id_list{l_id};

  detail::sql_com_destroy_parent_id<tables::project_config_assets_list>(in_ptr, l_main_id_list);
  detail::sql_com_destroy_parent_id<tables::project_config_icon_extensions>(in_ptr, l_main_id_list);
  detail::sql_com_destroy_parent_id<tables::project_config_maya_camera_select>(in_ptr, l_main_id_list);
  install_sub(in_ptr, l_id, in_config);
}
void sql_ctx<project_config::base_config>::select(
    const sql_connection_ptr& in_ptr, project_config::base_config& in_config
) {
  auto& l_conn = *in_ptr;

  std::int64_t l_parent_id{};
  {
    tables::project_config l_table{};

    for (auto& row : l_conn(sqlpp::select(
                                l_table.id, l_table.sim_path, l_table.export_group, l_table.cloth_proxy,
                                l_table.simple_module_proxy, l_table.find_icon_regex, l_table.upload_path,
                                l_table.season_count, l_table.use_only_sim_cloth, l_table.use_divide_group_export,
                                l_table.t_post, l_table.export_anim_time, l_table.abc_export_extract_reference_name,
                                l_table.abc_export_format_reference_name, l_table.abc_export_extract_scene_name,
                                l_table.abc_export_format_scene_name, l_table.abc_export_add_frame_range,
                                l_table.maya_camera_suffix, l_table.maya_out_put_abc_suffix, l_table.entity_id
         )
                                .from(l_table)
                                .where(l_table.entity_id.is_null()))) {
      in_config.vfx_cloth_sim_path                = row.sim_path.value();
      in_config.export_group                      = row.export_group.value();
      in_config.cloth_proxy_                      = row.cloth_proxy.value();
      in_config.simple_module_proxy_              = row.simple_module_proxy.value();
      in_config.find_icon_regex                   = row.find_icon_regex.value();
      in_config.upload_path                       = row.upload_path.value();
      in_config.season_count                      = row.season_count.value();
      in_config.use_only_sim_cloth                = row.use_only_sim_cloth.value();
      in_config.use_divide_group_export           = row.use_divide_group_export.value();
      in_config.t_post                            = row.t_post.value();
      in_config.export_anim_time                  = row.export_anim_time.value();
      in_config.abc_export_extract_reference_name = row.abc_export_extract_reference_name.value();
      in_config.abc_export_format_reference_name  = row.abc_export_format_reference_name.value();
      in_config.abc_export_extract_scene_name     = row.abc_export_extract_scene_name.value();
      in_config.abc_export_format_scene_name      = row.abc_export_format_scene_name.value();
      in_config.abc_export_add_frame_range        = row.abc_export_add_frame_range.value();
      in_config.maya_camera_suffix                = row.maya_camera_suffix.value();
      in_config.maya_out_put_abc_suffix           = row.maya_out_put_abc_suffix.value();
      auto l_id                                   = row.entity_id.value();
      l_parent_id                                 = row.id.value();
      // DOODLE_LOG_INFO("选择数据库id {} 插入上下文", l_id);
      break;
    }
  }

  {
    const tables::project_config_assets_list l_table{};
    auto l_pre =
        l_conn.prepare(sqlpp::select(l_table.assets_list).from(l_table).where(l_table.parent_id == l_parent_id));

    for (auto&& row : l_conn(l_pre)) {
      in_config.assets_list.emplace_back(row.assets_list.value());
    }
  }

  {
    const tables::project_config_icon_extensions l_table{};
    auto l_pre =
        l_conn.prepare(sqlpp::select(l_table.icon_extensions).from(l_table).where(l_table.parent_id == l_parent_id));

    for (auto&& row : l_conn(l_pre)) {
      in_config.icon_extensions.emplace_back(row.icon_extensions.value());
    }
  }
  {
    const tables::project_config_maya_camera_select l_table{};
    auto l_pre = l_conn.prepare(sqlpp::select(l_table.maya_camera_select_reg, l_table.maya_camera_select_num)
                                    .from(l_table)
                                    .where(l_table.parent_id == l_parent_id));

    for (auto&& row : l_conn(l_pre)) {
      in_config.maya_camera_select.emplace_back(
          row.maya_camera_select_reg.value(), boost::numeric_cast<std::int32_t>(row.maya_camera_select_num.value())
      );
    }
  }
}

}  // namespace doodle::database_n