#include "entity_path.h"

#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

namespace doodle {

FSys::path get_entity_character_rig_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("Ch/JD{:02d}_{:02d}/Ch{}/Rig", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}
FSys::path get_entity_character_rig_maya_path(const project& in_prj_, const entity_asset_extend& in_extend_) {
  if (in_prj_.asset_root_path_.empty() || !in_extend_.gui_dang_ || !in_extend_.kai_shi_ji_shu_ ||
      in_extend_.bian_hao_.empty())
    throw std::runtime_error("无法生成角色模型maya 绑定路径，缺少 资产根路径、归档、开始集或编号 信息");
  return get_entity_character_rig_maya_path(
      in_prj_.asset_root_path_, *in_extend_.gui_dang_, *in_extend_.kai_shi_ji_shu_, in_extend_.bian_hao_
  );
}
/// 角色模型maya 路径
FSys::path get_entity_character_model_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("Ch/JD{:02d}_{:02d}/Ch{}/Mod", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}

/// 道具模型maya 绑定路径
FSys::path get_entity_prop_rig_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& pin_yin_ming_cheng_
) {
  return asset_root_path_ / fmt::format("Prop/JD{:02d}_{:02d}/{}/Rig", gui_dang_, kai_shi_ji_shu_, pin_yin_ming_cheng_);
}
/// 道具模型maya 路径
FSys::path get_entity_prop_model_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& pin_yin_ming_cheng_
) {
  return asset_root_path_ / fmt::format("Prop/JD{:02d}_{:02d}/{}", gui_dang_, kai_shi_ji_shu_, pin_yin_ming_cheng_);
}
/// 场景模型maya 绑定路径
FSys::path get_entity_ground_rig_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("BG/JD{:02d}_{:02d}/BG{}/Mod", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}

/// 场景模型maya 路径
FSys::path get_entity_ground_model_maya_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("BG/JD{:02d}_{:02d}/BG{}/Mod", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}
/// 角色模型ue 路径
FSys::path get_entity_character_ue_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_
) {
  return asset_root_path_ /
         fmt::format("Ch/JD{:02d}_{:02d}/Ch{}/{}_UE5", gui_dang_, kai_shi_ji_shu_, bian_hao_, pin_yin_ming_cheng_);
}
/// 角色模型 ue 名称
std::string get_entity_character_ue_name(const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_) {
  return fmt::format(
      "{}/Character/{}/Meshs/SK_Ch{}.uasset", doodle_config::ue4_content, pin_yin_ming_cheng_, bian_hao_
  );
}
/// 道具模型ue 路径
FSys::path get_entity_prop_ue_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_
) {
  return asset_root_path_ /
         fmt::format("Prop/JD{:02d}_{:02d}/JD{:02d}_{:02d}_UE", gui_dang_, kai_shi_ji_shu_, gui_dang_, kai_shi_ji_shu_);
}
/// 道具模型 ue 名称
std::string get_entity_prop_ue_name(
    const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_, const std::string& ban_ben_
) {
  return fmt::format(
      "{}/Prop/{}/Mesh/SK_{}{}{}.uasset", doodle_config::ue4_content, pin_yin_ming_cheng_, bian_hao_,
      ban_ben_.empty() ? "" : "_", ban_ben_
  );
}
/// 场景模型ue 路径
FSys::path get_entity_ground_ue_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_
) {
  return asset_root_path_ /
         fmt::format("BG/JD{:02d}_{:02d}/BG{}/{}", gui_dang_, kai_shi_ji_shu_, bian_hao_, pin_yin_ming_cheng_);
}
/// 场景模型 ue map 名称
std::string get_entity_ground_ue_map_name(
    const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_, const std::string& ban_ben_
) {
  return fmt::format(
      "{}/{}/Map/{}{}{}.umap", doodle_config::ue4_content, pin_yin_ming_cheng_, pin_yin_ming_cheng_,
      ban_ben_.empty() ? "" : "_", ban_ben_
  );
}
///  场景模型 ue sk 名称
std::string get_entity_ground_ue_sk_name(
    const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_, const std::string& ban_ben_
) {
  return fmt::format(
      "{}/{}/SK/SK_{}{}{}.uasset", doodle_config::ue4_content, pin_yin_ming_cheng_, pin_yin_ming_cheng_,
      ban_ben_.empty() ? "" : "_", ban_ben_
  );
}
/// 场景名称 alembic 名称
std::string get_entity_ground_alembic_name(
    const std::string& bian_hao_, const std::string& pin_yin_ming_cheng_, const std::string& ban_ben_
) {
  return fmt::format("{}{}{}_Low.abc", pin_yin_ming_cheng_, ban_ben_.empty() ? "" : "_", ban_ben_);
}

/// 角色模型图片 路径
FSys::path get_entity_character_image_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("Ch/JD{:02d}_{:02d}/Ch{}", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}
/// 道具模型图片 路径
FSys::path get_entity_prop_image_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& pin_yin_ming_cheng_
) {
  return asset_root_path_ / fmt::format("Prop/JD{:02d}_{:02d}/{}", gui_dang_, kai_shi_ji_shu_, pin_yin_ming_cheng_);
}
/// 场景模型图片 路径
FSys::path get_entity_ground_image_path(
    const FSys::path& asset_root_path_, std::int32_t gui_dang_, std::int32_t kai_shi_ji_shu_,
    const std::string& bian_hao_
) {
  return asset_root_path_ / fmt::format("BG/JD{:02d}_{:02d}/BG{}", gui_dang_, kai_shi_ji_shu_, bian_hao_);
}
/// 获得解算资产路径
FSys::path get_entity_simulation_asset_path(const FSys::path& asset_root_path_) { return asset_root_path_ / "CFX"; }

/// 动画镜头maya路径
FSys::path get_shots_animation_maya_path(const std::string& episode_name_) {
  return FSys::path{"03_Workflow"} / "shots" / episode_name_ / "ma";
  ;
}
/// 动画镜头output路径
FSys::path get_shots_animation_output_path(
    const std::string& episode_name_, const std::string& shot_name_, const std::string& project_code_
) {
  return FSys::path{} / "03_Workflow" / "shots" / episode_name_ / "fbx" /
         fmt::format("{}_{}_{}", project_code_, episode_name_, shot_name_);
}
/// 解算镜头maya路径
FSys::path get_shots_simulation_maya_path(const std::string& episode_name_) {
  return FSys::path{"03_Workflow"} / "shots" / episode_name_ / "sim";
}
/// 解算镜头output路径
FSys::path get_shots_simulation_output_path(
    const std::string& episode_name_, const std::string& shot_name_, const std::string& project_code_
) {
  return FSys::path{} / "03_Workflow" / "shots" / episode_name_ / "abc" /
         fmt::format("{}_{}_{}", project_code_, episode_name_, shot_name_);
}
/// 动画文件名称
std::string get_shots_animation_file_name(
    const std::string& episode_name_, const std::string& shot_name_, const std::string& project_code_
) {
  return fmt::format("{}_{}_{}", project_code_, episode_name_, shot_name_);
}
FSys::path conv_ue_game_path(const FSys::path& in_path) {
  auto l_str = in_path.generic_string();
  boost::replace_all(l_str, doodle_config::ue4_content, doodle_config::ue4_game);
  FSys::path l_path{l_str};
  l_path.replace_extension(l_path.stem());
  return l_path;
}

}  // namespace doodle