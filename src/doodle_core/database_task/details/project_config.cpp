#include "project_config.h"

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/generate/core/sql_sql.h>
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
namespace sql = doodle_database;
void sql_com<project_config::base_config>::insert(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  sql::ProjectConfig l_table{};

  auto l_pre = l_conn.prepare(sqlpp::insert_into(l_table).set(
      l_table.simPath = sqlpp::parameter(l_table.simPath), l_table.exportGroup = sqlpp::parameter(l_table.exportGroup),
      l_table.clothProxy           = sqlpp::parameter(l_table.clothProxy),
      l_table.simpleModuleProxy    = sqlpp::parameter(l_table.simpleModuleProxy),
      l_table.findIconRegex        = sqlpp::parameter(l_table.findIconRegex),
      l_table.assetsList           = sqlpp::parameter(l_table.assetsList),
      l_table.iconExtensions       = sqlpp::parameter(l_table.iconExtensions),
      l_table.uploadPath           = sqlpp::parameter(l_table.uploadPath),
      l_table.seasonCount          = sqlpp::parameter(l_table.seasonCount),
      l_table.useOnlySimCloth      = sqlpp::parameter(l_table.useOnlySimCloth),
      l_table.useDivideGroupExport = sqlpp::parameter(l_table.useDivideGroupExport),
      l_table.useRenameMaterial    = sqlpp::parameter(l_table.useRenameMaterial),
      l_table.useMergeMesh = sqlpp::parameter(l_table.useMergeMesh), l_table.tPost = sqlpp::parameter(l_table.tPost),
      l_table.exportAnimTime                = sqlpp::parameter(l_table.exportAnimTime),
      l_table.exportAbcArg                  = sqlpp::parameter(l_table.exportAbcArg),
      l_table.mayaCameraSelect              = sqlpp::parameter(l_table.mayaCameraSelect),
      l_table.useWriteMetadata              = sqlpp::parameter(l_table.useWriteMetadata),
      l_table.abcExportExtractReferenceName = sqlpp::parameter(l_table.abcExportExtractReferenceName),
      l_table.abcExportFormatReferenceName  = sqlpp::parameter(l_table.abcExportFormatReferenceName),
      l_table.abcExportExtractSceneName     = sqlpp::parameter(l_table.abcExportExtractSceneName),
      l_table.abcExportFormatSceneName      = sqlpp::parameter(l_table.abcExportFormatSceneName),
      l_table.abcExportAddFrameRange        = sqlpp::parameter(l_table.abcExportAddFrameRange),
      l_table.mayaCameraSuffix              = sqlpp::parameter(l_table.mayaCameraSuffix),
      l_table.mayaOutPutAbcSuffix           = sqlpp::parameter(l_table.mayaOutPutAbcSuffix),
      l_table.entityId                      = sqlpp::parameter(l_table.entityId)
  ));

  for (auto& l_h : l_handles) {
    auto& l_pconfig                            = l_h.get<project_config::base_config>();
    l_pre.params.simPath                       = l_pconfig.vfx_cloth_sim_path.string();
    l_pre.params.exportGroup                   = l_pconfig.export_group;
    l_pre.params.clothProxy                    = l_pconfig.cloth_proxy_;
    l_pre.params.simpleModuleProxy             = l_pconfig.simple_module_proxy_;
    l_pre.params.findIconRegex                 = l_pconfig.find_icon_regex;
    // todo vector
    //  l_pre.params.assetsList=l_pconfig.assets_list;
    //  l_pre.params.iconExtensions=l_pconfig.icon_extensions;
    l_pre.params.uploadPath                    = l_pconfig.upload_path.string();
    l_pre.params.seasonCount                   = l_pconfig.season_count;
    l_pre.params.useOnlySimCloth               = l_pconfig.use_only_sim_cloth;
    l_pre.params.useDivideGroupExport          = l_pconfig.use_divide_group_export;
    l_pre.params.useRenameMaterial             = l_pconfig.use_rename_material;
    l_pre.params.useMergeMesh                  = l_pconfig.use_merge_mesh;
    l_pre.params.tPost                         = l_pconfig.t_post;
    l_pre.params.exportAnimTime                = l_pconfig.export_anim_time;
    // todo
    //  l_pre.params.exportAbcArg=l_pconfig.export_abc_arg;
    //  l_pre.params.mayaCameraSelect=l_pconfig.maya_camera_select;
    l_pre.params.useWriteMetadata              = l_pconfig.use_write_metadata;
    l_pre.params.abcExportExtractReferenceName = l_pconfig.abc_export_extract_reference_name;
    l_pre.params.abcExportFormatReferenceName  = l_pconfig.abc_export_format_reference_name;
    l_pre.params.abcExportExtractSceneName     = l_pconfig.abc_export_extract_scene_name;
    l_pre.params.abcExportFormatSceneName      = l_pconfig.abc_export_format_scene_name;
    l_pre.params.abcExportAddFrameRange        = l_pconfig.abc_export_add_frame_range;
    l_pre.params.mayaCameraSuffix              = l_pconfig.maya_camera_suffix;
    l_pre.params.mayaOutPutAbcSuffix           = l_pconfig.maya_out_put_abc_suffix;

    l_pre.params.entityId                      = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    auto l_r                                   = l_conn(l_pre);
    DOODLE_LOG_INFO(
        "插入数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(),
        rttr::type::get<project_config::base_config>().get_name()
    );
  }
}

void sql_com<project_config::base_config>::update(conn_ptr& in_ptr, const std::vector<entt::entity>& in_id) {
  namespace uuids = boost::uuids;
  auto& l_conn    = *in_ptr;
  auto l_handles  = in_id | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
  sql::ProjectConfig l_table{};

  auto l_pre = l_conn.prepare(
      sqlpp::update(l_table)
          .set(
              l_table.simPath                       = sqlpp::parameter(l_table.simPath),
              l_table.exportGroup                   = sqlpp::parameter(l_table.exportGroup),
              l_table.clothProxy                    = sqlpp::parameter(l_table.clothProxy),
              l_table.simpleModuleProxy             = sqlpp::parameter(l_table.simpleModuleProxy),
              l_table.findIconRegex                 = sqlpp::parameter(l_table.findIconRegex),
              l_table.assetsList                    = sqlpp::parameter(l_table.assetsList),
              l_table.iconExtensions                = sqlpp::parameter(l_table.iconExtensions),
              l_table.uploadPath                    = sqlpp::parameter(l_table.uploadPath),
              l_table.seasonCount                   = sqlpp::parameter(l_table.seasonCount),
              l_table.useOnlySimCloth               = sqlpp::parameter(l_table.useOnlySimCloth),
              l_table.useDivideGroupExport          = sqlpp::parameter(l_table.useDivideGroupExport),
              l_table.useRenameMaterial             = sqlpp::parameter(l_table.useRenameMaterial),
              l_table.useMergeMesh                  = sqlpp::parameter(l_table.useMergeMesh),
              l_table.tPost                         = sqlpp::parameter(l_table.tPost),
              l_table.exportAnimTime                = sqlpp::parameter(l_table.exportAnimTime),
              l_table.exportAbcArg                  = sqlpp::parameter(l_table.exportAbcArg),
              l_table.mayaCameraSelect              = sqlpp::parameter(l_table.mayaCameraSelect),
              l_table.useWriteMetadata              = sqlpp::parameter(l_table.useWriteMetadata),
              l_table.abcExportExtractReferenceName = sqlpp::parameter(l_table.abcExportExtractReferenceName),
              l_table.abcExportFormatReferenceName  = sqlpp::parameter(l_table.abcExportFormatReferenceName),
              l_table.abcExportExtractSceneName     = sqlpp::parameter(l_table.abcExportExtractSceneName),
              l_table.abcExportFormatSceneName      = sqlpp::parameter(l_table.abcExportFormatSceneName),
              l_table.abcExportAddFrameRange        = sqlpp::parameter(l_table.abcExportAddFrameRange),
              l_table.mayaCameraSuffix              = sqlpp::parameter(l_table.mayaCameraSuffix),
              l_table.mayaOutPutAbcSuffix           = sqlpp::parameter(l_table.mayaOutPutAbcSuffix)
          )
          .where(l_table.entityId == sqlpp::parameter(l_table.entityId))
  );
  for (auto& l_h : l_handles) {
    auto& l_pconfig                            = l_h.get<project_config::base_config>();
    l_pre.params.simPath                       = l_pconfig.vfx_cloth_sim_path.string();
    l_pre.params.exportGroup                   = l_pconfig.export_group;
    l_pre.params.clothProxy                    = l_pconfig.cloth_proxy_;
    l_pre.params.simpleModuleProxy             = l_pconfig.simple_module_proxy_;
    l_pre.params.findIconRegex                 = l_pconfig.find_icon_regex;
    // todo vector
    //  l_pre.params.assetsList=l_pconfig.assets_list;
    //  l_pre.params.iconExtensions=l_pconfig.icon_extensions;
    l_pre.params.uploadPath                    = l_pconfig.upload_path.string();
    l_pre.params.seasonCount                   = l_pconfig.season_count;
    l_pre.params.useOnlySimCloth               = l_pconfig.use_only_sim_cloth;
    l_pre.params.useDivideGroupExport          = l_pconfig.use_divide_group_export;
    l_pre.params.useRenameMaterial             = l_pconfig.use_rename_material;
    l_pre.params.useMergeMesh                  = l_pconfig.use_merge_mesh;
    l_pre.params.tPost                         = l_pconfig.t_post;
    l_pre.params.exportAnimTime                = l_pconfig.export_anim_time;
    // todo
    //  l_pre.params.exportAbcArg=l_pconfig.export_abc_arg;
    //  l_pre.params.mayaCameraSelect=l_pconfig.maya_camera_select;
    l_pre.params.useWriteMetadata              = l_pconfig.use_write_metadata;
    l_pre.params.abcExportExtractReferenceName = l_pconfig.abc_export_extract_reference_name;
    l_pre.params.abcExportFormatReferenceName  = l_pconfig.abc_export_format_reference_name;
    l_pre.params.abcExportExtractSceneName     = l_pconfig.abc_export_extract_scene_name;
    l_pre.params.abcExportFormatSceneName      = l_pconfig.abc_export_format_scene_name;
    l_pre.params.abcExportAddFrameRange        = l_pconfig.abc_export_add_frame_range;
    l_pre.params.mayaCameraSuffix              = l_pconfig.maya_camera_suffix;
    l_pre.params.mayaOutPutAbcSuffix           = l_pconfig.maya_out_put_abc_suffix;

    l_pre.params.entityId                      = boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());
    auto l_r                                   = l_conn(l_pre);
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
  sql::ProjectConfig l_table{};
  std::vector<project_config::base_config> l_config;
  std::vector<entt::entity> l_entts;
  // 调整内存
  for (auto&& raw :
       l_conn(sqlpp::select(sqlpp::count(l_table.entityId)).from(l_table).where(l_table.entityId.is_not_null()))) {
    l_config.reserve(raw.count.value());
    l_entts.reserve(raw.count.value());
    break;
  }

  for (auto& row :
       l_conn(sqlpp::select(
                  l_table.simPath, l_table.exportGroup, l_table.clothProxy, l_table.simpleModuleProxy,
                  l_table.findIconRegex, l_table.assetsList, l_table.iconExtensions, l_table.uploadPath,
                  l_table.seasonCount, l_table.useOnlySimCloth, l_table.useDivideGroupExport, l_table.useRenameMaterial,
                  l_table.useMergeMesh, l_table.tPost, l_table.exportAnimTime, l_table.exportAbcArg,
                  l_table.mayaCameraSelect, l_table.useWriteMetadata, l_table.abcExportExtractReferenceName,
                  l_table.abcExportFormatReferenceName, l_table.abcExportExtractSceneName,
                  l_table.abcExportFormatSceneName, l_table.abcExportAddFrameRange, l_table.mayaCameraSuffix,
                  l_table.mayaOutPutAbcSuffix, l_table.entityId
       )
                  .from(l_table)
                  .where(l_table.entityId.is_null()))) {
    project_config::base_config l_p_c{};
    l_p_c.vfx_cloth_sim_path                = row.simPath.value();
    l_p_c.export_group                      = row.exportGroup.value();
    l_p_c.cloth_proxy_                      = row.clothProxy.value();
    l_p_c.simple_module_proxy_              = row.simpleModuleProxy.value();
    l_p_c.find_icon_regex                   = row.findIconRegex.value();
    // l_p_c.assets_list=row.assetsList.value();
    // l_p_c.icon_extensions=row.iconExtensions.value();
    l_p_c.upload_path                       = row.uploadPath.value();
    l_p_c.season_count                      = row.seasonCount.value();
    l_p_c.use_only_sim_cloth                = row.useOnlySimCloth.value();
    l_p_c.use_divide_group_export           = row.useDivideGroupExport.value();
    l_p_c.use_rename_material               = row.useRenameMaterial.value();
    l_p_c.use_merge_mesh                    = row.useMergeMesh.value();
    l_p_c.t_post                            = row.tPost.value();
    l_p_c.export_anim_time                  = row.exportAnimTime.value();
    l_p_c.export_abc_arg                    = row.exportAbcArg.value();
    // l_p_c.maya_camera_select=row.mayaCameraSelect.value();
    l_p_c.use_write_metadata                = row.useWriteMetadata.value();
    l_p_c.abc_export_extract_reference_name = row.abcExportExtractReferenceName.value();
    l_p_c.abc_export_format_reference_name  = row.abcExportFormatReferenceName.value();
    l_p_c.abc_export_extract_scene_name     = row.abcExportExtractSceneName.value();
    l_p_c.abc_export_format_scene_name      = row.abcExportFormatSceneName.value();
    l_p_c.abc_export_add_frame_range        = row.abcExportAddFrameRange.value();
    l_p_c.maya_camera_suffix                = row.mayaCameraSuffix.value();
    l_p_c.maya_out_put_abc_suffix           = row.mayaOutPutAbcSuffix.value();
    auto l_id                               = row.entityId.value();
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
  detail::sql_com_destroy<sql::ProjectConfig>(in_ptr, in_handle);
}

}  // namespace doodle::database_n