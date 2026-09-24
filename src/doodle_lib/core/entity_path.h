#pragma once
#include "doodle_core/metadata/episodes.h"
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/project.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <bitset>
#include <filesystem>
#include <optional>
#include <string>

namespace doodle {

/// 角色模型maya 绑定路径
FSys::path get_entity_character_rig_maya_path(const project& in_prj_, const entity_asset_extend_value& in_extend_);
/// 角色模型maya 路径
FSys::path get_entity_character_model_maya_path(const project& in_prj_, const entity_asset_extend_value& in_extend_);
FSys::path get_entity_character_rig_maya_name(const entity_asset_extend_value& in_extend_);
/// 道具模型maya 绑定路径
FSys::path get_entity_prop_rig_maya_path(const project& in_prj_, const entity_asset_extend_value& in_extend_);
FSys::path get_entity_prop_rig_maya_name(const entity_asset_extend_value& in_extend_);
/// 道具模型maya 路径
FSys::path get_entity_prop_model_maya_path(const project& in_prj_, const entity_asset_extend_value& in_extend_);
FSys::path get_entity_prop_model_maya_name(const entity_asset_extend_value& in_extend_);
/// 场景模型maya 绑定路径
FSys::path get_entity_ground_rig_maya_path(const project& in_prj_, const entity_asset_extend_value& in_extend_);

/// 场景模型maya 路径
FSys::path get_entity_ground_model_maya_path(const project& in_prj_, const entity_asset_extend_value& in_extend_);
/// 角色模型ue 路径
FSys::path get_entity_character_ue_path(const project& in_prj_, const entity_asset_extend_value& in_extend_);
/// 角色模型 ue 名称
FSys::path get_entity_character_ue_name(const entity_asset_extend_value& in_extend_);
FSys::path get_entity_sim_character_ue_name(
    const entity_asset_extend_value& in_extend_, const std::bitset<2>& sim_type_ = simulation_status_cloth
);
/// 角色模型 ue groom 名称
FSys::path get_entity_character_ue_groom_name(
    const entity_asset_extend_value& in_extend_, std::string_view in_groom_name_,
    const std::bitset<2>& sim_type_ = simulation_status_none
);

/// 道具模型ue 路径
FSys::path get_entity_prop_ue_path(const project& in_prj_, const entity_asset_extend_value& in_extend_);
FSys::path get_entity_prop_ue_public_files_path();
FSys::path get_entity_prop_ue_files_path(const entity_asset_extend_value& in_extend_);
/// 道具模型 ue 名称
FSys::path get_entity_prop_ue_name(const entity_asset_extend_value& in_extend_);
FSys::path get_entity_sim_prop_ue_name(
    const entity_asset_extend_value& in_extend_, const std::bitset<2>& sim_type_ = simulation_status_cloth
);
/// 场景模型ue 路径
FSys::path get_entity_ground_ue_path(const project& in_prj_, const entity_asset_extend_value& in_extend_);
/// 场景模型 ue map 名称
FSys::path get_entity_ground_ue_map_name(const entity_asset_extend_value& in_extend_);
///  场景模型 ue sk 名称
FSys::path get_entity_ground_ue_sk_name(const entity_asset_extend_value& in_extend_);
FSys::path get_entity_sim_ground_ue_sk_name(
    const entity_asset_extend_value& in_extend_, const std::bitset<2>& sim_type_ = simulation_status_cloth
);
/// 场景名称 alembic 名称
FSys::path get_entity_ground_alembic_name(const entity_asset_extend_value& in_extend_);
FSys::path get_entity_ground_rig_name(const entity_asset_extend_value& in_extend_);
/// 角色模型图片 路径
FSys::path get_entity_character_image_path(const project& in_prj_, const entity_asset_extend_value& in_extend_);
/// 道具模型图片 路径
FSys::path get_entity_prop_image_path(const project& in_prj_, const entity_asset_extend_value& in_extend_);
/// 场景模型图片 路径
FSys::path get_entity_ground_image_path(const project& in_prj_, const entity_asset_extend_value& in_extend_);
/// 获得解算资产路径
FSys::path get_entity_simulation_asset_path(const project& in_prj_);
FSys::path get_entity_simulation_prop_asset_name(const entity_asset_extend_value& in_extend_);
FSys::path get_entity_simulation_character_asset_name(const entity_asset_extend_value& in_extend_);
/// 动画镜头maya路径
FSys::path get_shots_animation_maya_path(const entity& episode_);
/// 动画镜头output路径
FSys::path get_shots_animation_output_path(
    const std::string& episode_name_, const std::string& shot_name_, const std::string& project_code_
);
FSys::path get_shots_animation_output_path(const entity& episode_, const entity& shot_, const project& prj_);
/// 解算镜头maya路径
FSys::path get_shots_simulation_maya_path(const entity& episode_);
/// 解算镜头output路径
FSys::path get_shots_simulation_output_path(
    const std::string& episode_name_, const std::string& shot_name_, const std::string& project_code_
);
FSys::path get_shots_simulation_output_path(const entity& episode_, const entity& shot_, const project& prj_);
/// 动画文件名称
FSys::path get_shots_animation_file_name(
    const std::string& episode_name_, const std::string& shot_name_, const std::string& project_code_
);
FSys::path get_shots_animation_file_name(const entity& episode_, const entity& shot_, const project& prj_);
// 生成特效 ue路径
FSys::path get_shots_effect_ue_path(const entity& episode_);
// 生成灯光 ue路径
FSys::path get_shots_lighting_ue_path(const entity& episode_);
FSys::path get_shots_effect_movie_path(const entity& episode_);
FSys::path get_shots_lighting_movie_path(const entity& episode_);
FSys::path get_shots_ground_pretreatment_movie_path(const entity& episode_);
// 自动灯光上传文件夹
FSys::path get_shots_auto_lighting_upload_path(
    const episodes& episode_, const entity_asset_extend_value& ground_extend_
);
FSys::path get_shots_auto_lighting_upload_animation_name(
    const episodes& episode_, const shot& shot_, const std::string& project_code_
);
FSys::path get_shots_auto_lighting_upload_simulation_name(
    const episodes& episode_, const shot& shot_, const std::string& project_code_
);
FSys::path get_shots_auto_lighting_upload_movie_path(const episodes& episode_, const shot& shot_);

// ---------------------------------------------------------------------------
// AI 灯光（docs/ue_ai_lighting_plan.md）产物落盘
//
// 全部挂在自动灯光上传目录下再分 AI_Light/<会话 uuid>/，跟着项目一起归档：
//   <auto_lighting_upload>/AI_Light/<session_uuid>/ref/ref_00.png
//   <auto_lighting_upload>/AI_Light/<session_uuid>/it_001/cand_00.png
//   <auto_lighting_upload>/AI_Light/<session_uuid>/it_001/cand_00.exr
//
// 参考图与中间候选渲染都**不注册 preview_file**，由专用接口从这些路径直接提供；
// 只有定稿候选才注册 preview_file（source = auto_light_generate），
// 免得镜头预览列表被 6×N 张候选淹没。
// ---------------------------------------------------------------------------

/// AI 灯光会话根目录
FSys::path get_ai_light_session_path(
    const episodes& episode_, const entity_asset_extend_value& ground_extend_, const uuid& in_session_id
);
/// 会话的参考图目录
FSys::path get_ai_light_reference_path(
    const episodes& episode_, const entity_asset_extend_value& ground_extend_, const uuid& in_session_id
);
/// 某一轮的候选渲染目录
FSys::path get_ai_light_iteration_path(
    const episodes& episode_, const entity_asset_extend_value& ground_extend_, const uuid& in_session_id,
    std::int32_t in_iteration
);
/// 单个候选的渲染图路径。in_linear = true 取线性 EXR（不下发浏览器），false 取显示空间 PNG。
FSys::path get_ai_light_candidate_path(
    const episodes& episode_, const entity_asset_extend_value& ground_extend_, const uuid& in_session_id,
    std::int32_t in_iteration, std::int32_t in_candidate_index, bool in_linear = false
);

// 将路径转换为 ue_game 路径
FSys::path conv_ue_game_path(const FSys::path& in_path);
// 将 ue_game 路径转换为 正常路径
FSys::path conv_normal_path(const FSys::path& in_path);
FSys::path sk_conv_bone_name(const FSys::path& in_name);
}  // namespace doodle