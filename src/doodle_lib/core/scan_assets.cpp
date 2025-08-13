//
// Created by TD on 25-8-13.
//

#include "scan_assets.h"
namespace doodle::scan_assets {
namespace {
working_file create_working_file(
    const FSys::path& in_path, const std::string& in_description, software_enum in_software_type
) {
  working_file l_file{};
  if (exists(in_path)) {
    auto l_uuid = FSys::software_flag_file(in_path);
    if (l_uuid.is_nil()) {
      l_uuid = core_set::get_set().get_uuid();
      FSys::software_flag_file(in_path, l_uuid);
    }
    l_file = working_file{
        .uuid_id_       = l_uuid,
        .name_          = in_path.filename().generic_string(),
        .description_   = in_description,
        .size_          = boost::numeric_cast<std::int64_t>(FSys::file_size(in_path)),
        .path_          = in_path,
        .software_type_ = in_software_type,
    };
  }
  return l_file;
}

std::shared_ptr<working_file> scan_character_maya(const project& in_prj, const entity_asset_extend& in_extend) {
  FSys::path l_maya_path = in_prj.path_ / in_prj.asset_root_path_ / "Ch" /
                           fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) /
                           fmt::format("Ch{}", in_extend.bian_hao_) / "Mod" /
                           fmt::format("Ch{}.ma", in_extend.bian_hao_);
  if (exists(l_maya_path))
    return std::make_shared<working_file>(create_working_file(l_maya_path, "Maya文件", software_enum::maya));
  return nullptr;
}
std::shared_ptr<working_file> scan_character_unreal_engine(
    const project& in_prj, const entity_asset_extend& in_extend
) {
  FSys::path l_Ue_path = in_prj.path_ / in_prj.asset_root_path_ / "Ch" /
                         fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) /
                         fmt::format("Ch{}", in_extend.bian_hao_) /
                         fmt::format("{}_UE5", in_extend.pin_yin_ming_cheng_) / doodle_config::ue4_content /
                         "Character" / in_extend.pin_yin_ming_cheng_ / "Meshs" /
                         fmt::format("SK_Ch{}.uasset", in_extend.bian_hao_);
  if (exists(l_Ue_path))
    return std::make_shared<working_file>(create_working_file(l_Ue_path, "UE4角色模型", software_enum::unreal_engine));
  return nullptr;
}

std::shared_ptr<working_file> scan_prop_maya(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_maya_path =
      in_prj.path_ / in_prj.asset_root_path_ / "Prop" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) / in_extend.pin_yin_ming_cheng_ /
      fmt::format(
          "{}{}{}.ma", in_extend.pin_yin_ming_cheng_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_
      );
  if (exists(l_maya_path))
    return std::make_shared<working_file>(create_working_file(
        l_maya_path, fmt::format("Maya道具模型 {}", in_extend.pin_yin_ming_cheng_), software_enum::maya
    ));
  return nullptr;
}
std::shared_ptr<working_file> scan_prop_unreal_engine(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_UE_path =
      in_prj.path_ / in_prj.asset_root_path_ / "Prop" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) /
      fmt::format("JD{:02d}_{:02d}_UE", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) / doodle_config::ue4_content /
      "Prop" / in_extend.pin_yin_ming_cheng_ / "Mesh" /
      fmt::format(
          "SK_{}{}{}.uasset", in_extend.pin_yin_ming_cheng_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_
      );
  if (exists(l_UE_path))
    return std::make_shared<working_file>(create_working_file(
        l_UE_path, fmt::format("UE4道具模型 {}", in_extend.pin_yin_ming_cheng_), software_enum::unreal_engine
    ));
  return nullptr;
}

std::shared_ptr<working_file> scan_scene_maya(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_maya_path =
      in_prj.path_ / in_prj.asset_root_path_ / "BG" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) /
      fmt::format("BG{}", in_extend.bian_hao_) / "Mod" /
      fmt::format("BG{}{}{}.ma", in_extend.bian_hao_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_);
  if (exists(l_maya_path))
    return std::make_shared<working_file>(create_working_file(l_maya_path, "Maya场景文件", software_enum::maya));
  return nullptr;
}
std::shared_ptr<working_file> scan_scene_unreal_engine(const project& in_prj, const entity_asset_extend& in_extend) {
  auto l_UE_path =
      in_prj.path_ / in_prj.asset_root_path_ / "BG" /
      fmt::format("JD{:02d}_{:02d}", in_extend.gui_dang_, in_extend.kai_shi_ji_shu_) /
      fmt::format("BG{}", in_extend.bian_hao_) / in_extend.pin_yin_ming_cheng_ / doodle_config::ue4_content / "Map" /
      fmt::format(
          "{}{}{}.umap", in_extend.pin_yin_ming_cheng_, in_extend.ban_ben_.empty() ? "" : "_", in_extend.ban_ben_
      );
  if (exists(l_UE_path))
    return std::make_shared<working_file>(create_working_file(l_UE_path, "UE4场景文件", software_enum::unreal_engine));
  return nullptr;
}

}  // namespace
std::shared_ptr<working_file> scan(
    const project& in_prj, const uuid& in_entity_type, const entity_asset_extend& in_extend
) {
  std::vector<std::shared_ptr<working_file>> l_ret{};
  if (in_entity_type == asset_type::get_character_id()) return scan_character_maya(in_prj, in_extend);

  if (in_entity_type == asset_type::get_prop_id() || in_entity_type == asset_type::get_effect_id())
    return scan_prop_maya(in_prj, in_extend);

  if (in_entity_type == asset_type::get_scene_id()) return scan_scene_maya(in_prj, in_extend);

  return nullptr;
}
std::shared_ptr<working_file> scan_unreal_engine(
    const project& in_prj, const uuid& in_entity_type, const entity_asset_extend& in_extend
) {
  std::vector<std::shared_ptr<working_file>> l_ret{};
  if (in_entity_type == asset_type::get_character_id()) return scan_character_unreal_engine(in_prj, in_extend);

  if (in_entity_type == asset_type::get_prop_id() || in_entity_type == asset_type::get_effect_id()) {
    return scan_prop_unreal_engine(in_prj, in_extend);

    if (in_entity_type == asset_type::get_scene_id()) return scan_scene_unreal_engine(in_prj, in_extend);

    return nullptr;
  }
}

}  // namespace doodle::scan_assets