#include "project_config.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/logger/logger.h>

#include <boost/winapi/error_codes.hpp>

#include "metadata/metadata.h"
#include "metadata/project.h"
#include <algorithm>
#include <entt/entity/fwd.hpp>
#include <lib_warp/enum_template_tool.h>
#include <magic_enum.hpp>
#include <sqlpp11/aggregate_functions/count.h>
#include <sqlpp11/parameter.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::database_n {
void sql_com<project_config::base_config>::insert(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  auto& l_conn   = *in_ptr;
  auto l_handles = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  tables::project_config l_table{};

  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.sim_path                          = sqlpp::parameter(l_table.sim_path),
      l_table.export_group                      = sqlpp::parameter(l_table.export_group),
      l_table.cloth_proxy                       = sqlpp::parameter(l_table.cloth_proxy),
      l_table.simple_module_proxy               = sqlpp::parameter(l_table.simple_module_proxy),
      l_table.find_icon_regex                   = sqlpp::parameter(l_table.find_icon_regex),
      l_table.assets_list                       = sqlpp::parameter(l_table.assets_list),
      l_table.icon_extensions                   = sqlpp::parameter(l_table.icon_extensions),
      l_table.upload_path                       = sqlpp::parameter(l_table.upload_path),
      l_table.season_count                      = sqlpp::parameter(l_table.season_count),
      l_table.use_only_sim_cloth                = sqlpp::parameter(l_table.use_only_sim_cloth),
      l_table.use_divide_group_export           = sqlpp::parameter(l_table.use_divide_group_export),
      l_table.use_rename_material               = sqlpp::parameter(l_table.use_rename_material),
      l_table.use_merge_mesh                    = sqlpp::parameter(l_table.use_merge_mesh),
      l_table.t_post                            = sqlpp::parameter(l_table.t_post),
      l_table.export_anim_time                  = sqlpp::parameter(l_table.export_anim_time),
      l_table.export_abc_arg                    = sqlpp::parameter(l_table.export_abc_arg),
      l_table.maya_camera_select                = sqlpp::parameter(l_table.maya_camera_select),
      l_table.use_write_metadata                = sqlpp::parameter(l_table.use_write_metadata),
      l_table.abc_export_extract_reference_name = sqlpp::parameter(l_table.abc_export_extract_reference_name),
      l_table.abc_export_format_reference_name  = sqlpp::parameter(l_table.abc_export_format_reference_name),
      l_table.abc_export_extract_scene_name     = sqlpp::parameter(l_table.abc_export_extract_scene_name),
      l_table.abc_export_format_scene_name      = sqlpp::parameter(l_table.abc_export_format_scene_name),
      l_table.abc_export_add_frame_range        = sqlpp::parameter(l_table.abc_export_add_frame_range),
      l_table.maya_camera_suffix                = sqlpp::parameter(l_table.maya_camera_suffix),
      l_table.maya_out_put_abc_suffix           = sqlpp::parameter(l_table.maya_out_put_abc_suffix),
      l_table.entity_id                         = sqlpp::parameter(l_table.entity_id)
  ));

  for (auto& l_h : l_handles) {
    auto& l_pconfig                                = l_h.get<project_config::base_config>();
    l_pre.params.sim_path                          = l_pconfig.vfx_cloth_sim_path.string();
    l_pre.params.export_group                      = l_pconfig.export_group;
    l_pre.params.cloth_proxy                       = l_pconfig.cloth_proxy_;
    l_pre.params.simple_module_proxy               = l_pconfig.simple_module_proxy_;
    l_pre.params.find_icon_regex                   = l_pconfig.find_icon_regex;
    // todo vector
    //  l_pre.params.assets_list=l_pconfig.assets_list;
    //  l_pre.params.icon_extensions=l_pconfig.icon_extensions;
    l_pre.params.upload_path                       = l_pconfig.upload_path.string();
    l_pre.params.season_count                      = l_pconfig.season_count;
    l_pre.params.use_only_sim_cloth                = l_pconfig.use_only_sim_cloth;
    l_pre.params.use_divide_group_export           = l_pconfig.use_divide_group_export;
    l_pre.params.use_rename_material               = l_pconfig.use_rename_material;
    l_pre.params.use_merge_mesh                    = l_pconfig.use_merge_mesh;
    l_pre.params.t_post                            = l_pconfig.t_post;
    l_pre.params.export_anim_time                  = l_pconfig.export_anim_time;
    // todo
    //  l_pre.params.export_abc_arg=l_pconfig.export_abc_arg;
    //  l_pre.params.maya_camera_select=l_pconfig.maya_camera_select;
    l_pre.params.use_write_metadata                = l_pconfig.use_write_metadata;
    l_pre.params.abc_export_extract_reference_name = l_pconfig.abc_export_extract_reference_name;
    l_pre.params.abc_export_format_reference_name  = l_pconfig.abc_export_format_reference_name;
    l_pre.params.abc_export_extract_scene_name     = l_pconfig.abc_export_extract_scene_name;
    l_pre.params.abc_export_format_scene_name      = l_pconfig.abc_export_format_scene_name;
    l_pre.params.abc_export_add_frame_range        = l_pconfig.abc_export_add_frame_range;
    l_pre.params.maya_camera_suffix                = l_pconfig.maya_camera_suffix;
    l_pre.params.maya_out_put_abc_suffix           = l_pconfig.maya_out_put_abc_suffix;

    l_pre.params.entity_id                         = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    auto l_r                                       = l_conn(l_pre);
    DOODLE_LOG_INFO(
        "插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(),
        rttr::type::get<project_config::base_config>().get_name()
    );
  }
}

void sql_com<project_config::base_config>::update(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  auto& l_conn   = *in_ptr;
  auto l_handles = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  tables::project_config l_table{};

  auto l_pre = l_conn.prepare(
      sqlpp::update(l_table)
          .set(
              l_table.sim_path                          = sqlpp::parameter(l_table.sim_path),
              l_table.export_group                      = sqlpp::parameter(l_table.export_group),
              l_table.cloth_proxy                       = sqlpp::parameter(l_table.cloth_proxy),
              l_table.simple_module_proxy               = sqlpp::parameter(l_table.simple_module_proxy),
              l_table.find_icon_regex                   = sqlpp::parameter(l_table.find_icon_regex),
              l_table.assets_list                       = sqlpp::parameter(l_table.assets_list),
              l_table.icon_extensions                   = sqlpp::parameter(l_table.icon_extensions),
              l_table.upload_path                       = sqlpp::parameter(l_table.upload_path),
              l_table.season_count                      = sqlpp::parameter(l_table.season_count),
              l_table.use_only_sim_cloth                = sqlpp::parameter(l_table.use_only_sim_cloth),
              l_table.use_divide_group_export           = sqlpp::parameter(l_table.use_divide_group_export),
              l_table.use_rename_material               = sqlpp::parameter(l_table.use_rename_material),
              l_table.use_merge_mesh                    = sqlpp::parameter(l_table.use_merge_mesh),
              l_table.t_post                            = sqlpp::parameter(l_table.t_post),
              l_table.export_anim_time                  = sqlpp::parameter(l_table.export_anim_time),
              l_table.export_abc_arg                    = sqlpp::parameter(l_table.export_abc_arg),
              l_table.maya_camera_select                = sqlpp::parameter(l_table.maya_camera_select),
              l_table.use_write_metadata                = sqlpp::parameter(l_table.use_write_metadata),
              l_table.abc_export_extract_reference_name = sqlpp::parameter(l_table.abc_export_extract_reference_name),
              l_table.abc_export_format_reference_name  = sqlpp::parameter(l_table.abc_export_format_reference_name),
              l_table.abc_export_extract_scene_name     = sqlpp::parameter(l_table.abc_export_extract_scene_name),
              l_table.abc_export_format_scene_name      = sqlpp::parameter(l_table.abc_export_format_scene_name),
              l_table.abc_export_add_frame_range        = sqlpp::parameter(l_table.abc_export_add_frame_range),
              l_table.maya_camera_suffix                = sqlpp::parameter(l_table.maya_camera_suffix),
              l_table.maya_out_put_abc_suffix           = sqlpp::parameter(l_table.maya_out_put_abc_suffix)
          )
          .where(l_table.entity_id == sqlpp::parameter(l_table.entity_id))
  );
  for (auto& l_h : l_handles) {
    auto& l_pconfig                                = l_h.get<project_config::base_config>();
    l_pre.params.sim_path                          = l_pconfig.vfx_cloth_sim_path.string();
    l_pre.params.export_group                      = l_pconfig.export_group;
    l_pre.params.cloth_proxy                       = l_pconfig.cloth_proxy_;
    l_pre.params.simple_module_proxy               = l_pconfig.simple_module_proxy_;
    l_pre.params.find_icon_regex                   = l_pconfig.find_icon_regex;
    // todo vector
    //  l_pre.params.assets_list=l_pconfig.assets_list;
    //  l_pre.params.icon_extensions=l_pconfig.icon_extensions;
    l_pre.params.upload_path                       = l_pconfig.upload_path.string();
    l_pre.params.season_count                      = l_pconfig.season_count;
    l_pre.params.use_only_sim_cloth                = l_pconfig.use_only_sim_cloth;
    l_pre.params.use_divide_group_export           = l_pconfig.use_divide_group_export;
    l_pre.params.use_rename_material               = l_pconfig.use_rename_material;
    l_pre.params.use_merge_mesh                    = l_pconfig.use_merge_mesh;
    l_pre.params.t_post                            = l_pconfig.t_post;
    l_pre.params.export_anim_time                  = l_pconfig.export_anim_time;
    // todo
    //  l_pre.params.export_abc_arg=l_pconfig.export_abc_arg;
    //  l_pre.params.maya_camera_select=l_pconfig.maya_camera_select;
    l_pre.params.use_write_metadata                = l_pconfig.use_write_metadata;
    l_pre.params.abc_export_extract_reference_name = l_pconfig.abc_export_extract_reference_name;
    l_pre.params.abc_export_format_reference_name  = l_pconfig.abc_export_format_reference_name;
    l_pre.params.abc_export_extract_scene_name     = l_pconfig.abc_export_extract_scene_name;
    l_pre.params.abc_export_format_scene_name      = l_pconfig.abc_export_format_scene_name;
    l_pre.params.abc_export_add_frame_range        = l_pconfig.abc_export_add_frame_range;
    l_pre.params.maya_camera_suffix                = l_pconfig.maya_camera_suffix;
    l_pre.params.maya_out_put_abc_suffix           = l_pconfig.maya_out_put_abc_suffix;

    l_pre.params.entity_id                         = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    auto l_r                                       = l_conn(l_pre);
    DOODLE_LOG_INFO(
        "插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(),
        rttr::type::get<project_config::base_config>().get_name()
    );
  }
}
void sql_com<project_config::base_config>::select(
    conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle
) {
  auto& l_conn = *in_ptr;
  tables::project_config l_table{};
  std::vector<project_config::base_config> l_config;
  std::vector<entt::entity> l_entts;
  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entity_id)).from(l_table).where(l_table.entity_id.is_not_null()))) {
    l_config.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  for (auto& row :
       l_conn(sqlpp::select(
                  l_table.sim_path, l_table.export_group, l_table.cloth_proxy, l_table.simple_module_proxy,
                  l_table.find_icon_regex, l_table.assets_list, l_table.icon_extensions, l_table.upload_path,
                  l_table.season_count, l_table.use_only_sim_cloth, l_table.use_divide_group_export,
                  l_table.use_rename_material, l_table.use_merge_mesh, l_table.t_post, l_table.export_anim_time,
                  l_table.export_abc_arg, l_table.maya_camera_select, l_table.use_write_metadata,
                  l_table.abc_export_extract_reference_name, l_table.abc_export_format_reference_name,
                  l_table.abc_export_extract_scene_name, l_table.abc_export_format_scene_name,
                  l_table.abc_export_add_frame_range, l_table.maya_camera_suffix, l_table.maya_out_put_abc_suffix,
                  l_table.entity_id
       )
                  .from(l_table)
                  .where(l_table.entity_id.is_null()))) {
    project_config::base_config l_p_c{};
    l_p_c.vfx_cloth_sim_path                = row.sim_path.value();
    l_p_c.export_group                      = row.export_group.value();
    l_p_c.cloth_proxy_                      = row.cloth_proxy.value();
    l_p_c.simple_module_proxy_              = row.simple_module_proxy.value();
    l_p_c.find_icon_regex                   = row.find_icon_regex.value();
    // l_p_c.assets_list=row.assets_list.value();
    // l_p_c.icon_extensions=row.icon_extensions.value();
    l_p_c.upload_path                       = row.upload_path.value();
    l_p_c.season_count                      = row.season_count.value();
    l_p_c.use_only_sim_cloth                = row.use_only_sim_cloth.value();
    l_p_c.use_divide_group_export           = row.use_divide_group_export.value();
    l_p_c.use_rename_material               = row.use_rename_material.value();
    l_p_c.use_merge_mesh                    = row.use_merge_mesh.value();
    l_p_c.t_post                            = row.t_post.value();
    l_p_c.export_anim_time                  = row.export_anim_time.value();
    l_p_c.export_abc_arg                    = row.export_abc_arg.value();
    // l_p_c.maya_camera_select=row.maya_camera_select.value();
    l_p_c.use_write_metadata                = row.use_write_metadata.value();
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
      DOODLE_LOG_INFO("选择数据库id {} 插入实体 {}", l_id, in_handle.at(l_id));
    } else {
      DOODLE_LOG_INFO("选择数据库id {} 未找到实体", l_id);
    }
  }
  reg_->insert(l_entts.begin(), l_entts.end(), l_config.begin());
}
void sql_com<project_config::base_config>::destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle) {
  detail::sql_com_destroy<tables::project_config>(in_ptr, in_handle);
}

}  // namespace doodle::database_n