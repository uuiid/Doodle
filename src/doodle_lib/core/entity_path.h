#pragma once
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/project.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <filesystem>
#include <string>

namespace doodle {
/// 角色模型maya 绑定路径
FSys::path get_entity_character_rig_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
);
FSys::path get_entity_character_rig_maya_path(const project& in_prj_, const entity_asset_extend& in_extend_);
/// 角色模型maya 路径
FSys::path get_entity_character_model_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
);
FSys::path get_entity_character_model_maya_path(const project& in_prj_, const entity_asset_extend& in_extend_);
FSys::path get_entity_character_rig_maya_name(const entity_asset_extend& in_extend_);
/// 道具模型maya 绑定路径
FSys::path get_entity_prop_rig_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& pin_yin_ming_cheng_
);
FSys::path get_entity_prop_rig_maya_path(const project& in_prj_, const entity_asset_extend& in_extend_);
FSys::path get_entity_prop_rig_maya_name(const entity_asset_extend& in_extend_);
/// 道具模型maya 路径
FSys::path get_entity_prop_model_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& pin_yin_ming_cheng_
);
FSys::path get_entity_prop_model_maya_path(const project& in_prj_, const entity_asset_extend& in_extend_);
FSys::path get_entity_prop_model_maya_name(const entity_asset_extend& in_extend_);
/// 场景模型maya 绑定路径
FSys::path get_entity_ground_rig_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
);
FSys::path get_entity_ground_rig_maya_path(const project& in_prj_, const entity_asset_extend& in_extend_);

/// 场景模型maya 路径
FSys::path get_entity_ground_model_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
);
FSys::path get_entity_ground_model_maya_path(const project& in_prj_, const entity_asset_extend& in_extend_);
/// 角色模型ue 路径
FSys::path get_entity_character_ue_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_
);
FSys::path get_entity_character_ue_path(const project& in_prj_, const entity_asset_extend& in_extend_);

/// 角色模型 ue 名称
FSys::path get_entity_character_ue_name(const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_);
FSys::path get_entity_character_ue_name(const entity_asset_extend& in_extend_);
/// 道具模型ue 路径
FSys::path get_entity_prop_ue_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_
);
FSys::path get_entity_prop_ue_path(const project& in_prj_, const entity_asset_extend& in_extend_);
FSys::path get_entity_prop_ue_public_files_path();
FSys::path get_entity_prop_ue_files_path(const entity_asset_extend& in_extend_);
/// 道具模型 ue 名称
FSys::path get_entity_prop_ue_name(
    const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_, const std::string& ban_ben_
);
FSys::path get_entity_prop_ue_name(const entity_asset_extend& in_extend_);
/// 场景模型ue 路径
FSys::path get_entity_ground_ue_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_
);
FSys::path get_entity_ground_ue_path(const project& in_prj_, const entity_asset_extend& in_extend_);
/// 场景模型 ue map 名称
FSys::path get_entity_ground_ue_map_name(const std::string& pin_yin_ming_cheng_, const std::string& ban_ben_);
FSys::path get_entity_ground_ue_map_name(const entity_asset_extend& in_extend_);
///  场景模型 ue sk 名称
FSys::path get_entity_ground_ue_sk_name(const std::string& pin_yin_ming_cheng_, const std::string& ban_ben_);
FSys::path get_entity_ground_ue_sk_name(const entity_asset_extend& in_extend_);
/// 场景名称 alembic 名称
FSys::path get_entity_ground_alembic_name(const std::string& pin_yin_ming_cheng_, const std::string& ban_ben_);
FSys::path get_entity_ground_alembic_name(const entity_asset_extend& in_extend_);
FSys::path get_entity_ground_rig_name(const entity_asset_extend& in_extend_);
/// 角色模型图片 路径
FSys::path get_entity_character_image_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
);
FSys::path get_entity_character_image_path(const project& in_prj_, const entity_asset_extend& in_extend_);
/// 道具模型图片 路径
FSys::path get_entity_prop_image_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& pin_yin_ming_cheng_
);
FSys::path get_entity_prop_image_path(const project& in_prj_, const entity_asset_extend& in_extend_);
/// 场景模型图片 路径
FSys::path get_entity_ground_image_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
);
FSys::path get_entity_ground_image_path(const project& in_prj_, const entity_asset_extend& in_extend_);
/// 获得解算资产路径
FSys::path get_entity_simulation_asset_path(const FSys::path& asset_root_path_);
FSys::path get_entity_simulation_asset_path(const project& in_prj_);
FSys::path get_entity_simulation_prop_asset_name(const entity_asset_extend& in_extend_);
FSys::path get_entity_simulation_character_asset_name(const entity_asset_extend& in_extend_);
/// 动画镜头maya路径
FSys::path get_shots_animation_maya_path(const std::string& episode_name_);
FSys::path get_shots_animation_maya_path(const entity& episode_);
/// 动画镜头output路径
FSys::path get_shots_animation_output_path(
    const std::string& episode_name_, const std::string& shot_name_, const std::string& project_code_
);
FSys::path get_shots_animation_output_path(const entity& episode_, const entity& shot_, const project& prj_);
/// 解算镜头maya路径
FSys::path get_shots_simulation_maya_path(const std::string& episode_name_);
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
FSys::path conv_ue_game_path(const FSys::path& in_path);
void sk_conv_bone_name(FSys::path& in_name);
}  // namespace doodle