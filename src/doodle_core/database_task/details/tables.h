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

DOODLE_SQL_TABLE_IMP(file_association_ref, column::id, column::entity_id, column::ref_id);

namespace column_file_association {
DOODLE_SQL_COLUMN_IMP(entity_maya_file, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(entity_maya_rig_file, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(entity_maya_sim_file, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(entity_ue_file, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(entity_ue_preset_file, sqlpp::integer, detail::can_be_null);
}  // namespace column_file_association
DOODLE_SQL_TABLE_IMP(
    file_association, column::id, column::entity_id, column::name, column_file_association::entity_maya_file,
    column_file_association::entity_maya_rig_file, column_file_association::entity_ue_file,
    column_file_association::entity_ue_preset_file, column_file_association::entity_maya_sim_file
);
namespace column_tag {
DOODLE_SQL_COLUMN_IMP(tag_id, sqlpp::integer, detail::can_be_null);
}
DOODLE_SQL_TABLE_IMP(tag_table, column::id, column::entity_id, column_tag::tag_id);

namespace column_ue_main_map {
DOODLE_SQL_COLUMN_IMP(map_path_, sqlpp::text, detail::can_be_null);

}
DOODLE_SQL_TABLE_IMP(ue_main_map, column::id, column::entity_id, column_ue_main_map::map_path_);

namespace column_maya_anim_file {
DOODLE_SQL_COLUMN_IMP(begin_frame_, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(end_frame_, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(camera_path_, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(path_, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(ref_file_, sqlpp::integer, detail::can_be_null);

}  // namespace column_maya_anim_file

DOODLE_SQL_TABLE_IMP(
    maya_anim_file, column::id, column::entity_id, column_maya_anim_file::begin_frame_,
    column_maya_anim_file::end_frame_, column_maya_anim_file::camera_path_
);
DOODLE_SQL_TABLE_IMP(
    maya_anim_file_ref_file, column::id, column::parent_id, column_maya_anim_file::path_,
    column_maya_anim_file::ref_file_
);
namespace column_server_task_info {
DOODLE_SQL_COLUMN_IMP(data, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(status, sqlpp::integer, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(name, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(source_computer, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(submitter, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(submit_time, sqlpp::time_point, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(run_computer, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(run_computer_ip, sqlpp::text, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(run_time, sqlpp::time_point, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(end_time, sqlpp::time_point, detail::can_be_null);
DOODLE_SQL_COLUMN_IMP(log_path, sqlpp::text, detail::can_be_null);
}  // namespace column_server_task_info
DOODLE_SQL_TABLE_IMP(
    server_task_info, column::id, column::entity_id, column_server_task_info::data, column_server_task_info::status,
    column_server_task_info::name, column_server_task_info::source_computer, column_server_task_info::submitter,
    column_server_task_info::submit_time, column_server_task_info::run_computer,
    column_server_task_info::run_computer_ip, column_server_task_info::run_time, column_server_task_info::end_time,
    column_server_task_info::log_path
);
}  // namespace doodle::database_n::tables
