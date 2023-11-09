//
// Created by td_main on 2023/7/12.
//

#pragma once

#include <doodle_core/database_task/details/column.h>

#include <sqlpp11/data_types/boolean/data_type.h>
#include <sqlpp11/data_types/integral/data_type.h>
#include <sqlpp11/data_types/text/data_type.h>
#include <sqlpp11/data_types/time_point/data_type.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::database_n::tables {

DOODLE_SQL_TABLE_IMP(entity, column::id, column::uuid_data);
DOODLE_SQL_TABLE_IMP(com_entity, column::id, column::entity_id, column::com_hash, column::json_data);

DOODLE_SQL_TABLE_IMP(usertab, column::id, column::entity_id, column::user_name, column::permission_group);
DOODLE_SQL_TABLE_IMP(
    work_task_info, column::id, column::entity_id, column::ref_id, column::task_name, column::region, column::abstract,
    column::time_point
);
DOODLE_SQL_TABLE_IMP(episodes, column::id, column::entity_id, column::eps);
DOODLE_SQL_TABLE_IMP(shot, column::id, column::entity_id, column::shot_int, column::shot_ab);
DOODLE_SQL_TABLE_IMP(redirection_path_info, column::id, column::entity_id, column::redirection_file_name);
DOODLE_SQL_TABLE_IMP(rpi_search_path, column::id, column::parent_id, column::redirection_path);
DOODLE_SQL_TABLE_IMP(assets, column::id, column::entity_id, column::assets_path, column::parent_id);
DOODLE_SQL_TABLE_IMP(comment, column::id, column::entity_id, column::comment_string, column::comment_time);
DOODLE_SQL_TABLE_IMP(
    export_file_info, column::id, column::entity_id, column::file_path, column::start_frame, column::end_frame,
    column::ref_file, column::export_type_
);
DOODLE_SQL_TABLE_IMP(image_icon, column::id, column::entity_id, column::path);

namespace assets_file2_column {
DOODLE_SQL_COLUMN_IMP(file_association_ref_id, sqlpp::integer, detail::can_be_null);
}
DOODLE_SQL_TABLE_IMP(
    assets_file, column::id, column::entity_id, column::name, column::path, column::version, column::ref_id,
    column::organization, column::assets_ref_id, assets_file2_column::file_association_ref_id
);

DOODLE_SQL_TABLE_IMP(
    time_point_info, column::id, column::entity_id, column::first_time, column::second_time, column::info,
    column::is_extra_work
);
DOODLE_SQL_TABLE_IMP(
    project_config, column::id, column::entity_id, column::sim_path, column::export_group, column::cloth_proxy,
    column::simple_module_proxy, column::find_icon_regex, column::upload_path, column::season_count,
    column::use_only_sim_cloth, column::use_divide_group_export, column::t_post, column::export_anim_time,
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
DOODLE_SQL_TABLE_IMP(maya_file, column::id, column::entity_id, column::path);
DOODLE_SQL_TABLE_IMP(ue_file, column::id, column::entity_id, column::path);
DOODLE_SQL_TABLE_IMP(maya_rig_file, column::id, column::entity_id, column::path);
DOODLE_SQL_TABLE_IMP(ue_file_preset, column::id, column::entity_id, column::path);
namespace column_file_association {
DOODLE_SQL_COLUMN_IMP(entity_maya_file, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(entity_maya_rig_file, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(entity_ue_file, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(entity_ue_preset_file, sqlpp::integer, detail::can_be_null);
}  // namespace column_file_association
DOODLE_SQL_TABLE_IMP(
    file_association, column::id, column::entity_id, column::name, column_file_association::entity_maya_file,
    column_file_association::entity_maya_rig_file, column_file_association::entity_ue_file,
    column_file_association::entity_ue_preset_file
);
}  // namespace doodle::database_n::tables
