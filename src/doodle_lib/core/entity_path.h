#pragma once
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/project.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <filesystem>
#include <memory>

namespace doodle {
/// 角色模型maya 绑定路径
inline FSys::path get_entity_asset_rig_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("Ch/JD{:02d}_{:02d}/Ch{}/Rig", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}
/// 角色模型maya 路径
inline FSys::path get_entity_asset_model_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("Ch/JD{:02d}_{:02d}/Ch{}/Mod", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}

/// 道具模型maya 绑定路径
inline FSys::path get_entity_prop_rig_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& pin_yin_ming_cheng_
) {
  return asset_root_path_ / fmt::format("Prop/JD{:02d}_{:02d}/{}/Rig", gui_dang_, kai_shi_ji_shu_, pin_yin_ming_cheng_);
}
/// 道具模型maya 路径
inline FSys::path get_entity_prop_model_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& pin_yin_ming_cheng_
) {
  return asset_root_path_ / fmt::format("Prop/JD{:02d}_{:02d}/{}", gui_dang_, kai_shi_ji_shu_, pin_yin_ming_cheng_);
}
/// 场景模型maya 绑定路径
inline FSys::path get_entity_ground_rig_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("BG/JD{:02d}_{:02d}/BG{}/Mod", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}

/// 场景模型maya 路径
inline FSys::path get_entity_ground_model_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("BG/JD{:02d}_{:02d}/BG{}/Mod", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}
/// 角色模型ue 路径
inline FSys::path get_entity_asset_ue_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_
) {
  return asset_root_path_ /
         fmt::format("Ch/JD{:02d}_{:02d}/Ch{}/{}_UE5", gui_dang_, kai_shi_ji_shu_, bian_hao_, pin_yin_ming_cheng_);
}
/// 道具模型ue 路径
inline FSys::path get_entity_prop_ue_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_
) {
  return asset_root_path_ /
         fmt::format("Prop/JD{:02d}_{:02d}/JD{:02d}_{:02d}_UE", gui_dang_, kai_shi_ji_shu_, gui_dang_, kai_shi_ji_shu_);
}
/// 场景模型ue 路径
inline FSys::path get_entity_ground_ue_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_
) {
  return asset_root_path_ /
         fmt::format("BG/JD{:02d}_{:02d}/BG{}/{}", gui_dang_, kai_shi_ji_shu_, bian_hao_, pin_yin_ming_cheng_);
}
/// 角色模型图片 路径
inline FSys::path get_entity_asset_image_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("Ch/JD{:02d}_{:02d}/Ch{}", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}
/// 道具模型图片 路径
inline FSys::path get_entity_prop_image_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& pin_yin_ming_cheng_
) {
  return asset_root_path_ / fmt::format("Prop/JD{:02d}_{:02d}/{}", gui_dang_, kai_shi_ji_shu_, pin_yin_ming_cheng_);
}
/// 场景模型图片 路径
inline FSys::path get_entity_ground_image_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("BG/JD{:02d}_{:02d}/BG{}", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}

/// 动画镜头maya路径
inline FSys::path get_shots_animation_maya_path(const std::string& episode_name_) {
  return FSys::path{"03_Workflow"} / "shots" / episode_name_ / "ma";
  ;
}
/// 动画镜头output路径
inline FSys::path get_shots_animation_output_path(
    const std::string& episode_name_, const std::string& shot_name_, const std::string& project_code_
) {
  return FSys::path{} / "03_Workflow" / "shots" / episode_name_ / "fbx" /
         fmt::format("{}_{}_{}", project_code_, episode_name_, shot_name_);
}
/// 解算镜头maya路径
inline FSys::path get_shots_simulation_maya_path(const std::string& episode_name_) {
  return FSys::path{"03_Workflow"} / "shots" / episode_name_ / "sim";
}
/// 解算镜头output路径
inline FSys::path get_shots_simulation_output_path(
    const std::string& episode_name_, const std::string& shot_name_, const std::string& project_code_
) {
  return FSys::path{} / "03_Workflow" / "shots" / episode_name_ / "abc" /
         fmt::format("{}_{}_{}", project_code_, episode_name_, shot_name_);
}

}  // namespace doodle