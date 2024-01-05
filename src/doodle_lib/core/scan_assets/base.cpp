//
// Created by TD on 2023/12/20.
//
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/file_association.h>
#include <doodle_core/metadata/main_map.h>

#include <core/scan_assets/base.h>

namespace doodle::details {

entt::handle scan_category_data_t::get_project_handle() const {
  auto l_proect_view = g_reg()->view<project, assets>().each();
  for (auto&& [e, l_p, _] : l_proect_view) {
    if (l_p == project_root_) {
      return entt::handle{*g_reg(), e};
    }
  }

  entt::handle l_handle{*g_reg(), g_reg()->create()};
  l_handle.emplace<project>(project_root_);
  l_handle.emplace<assets>(project_root_.p_name);
  l_handle.emplace<database>();
  return l_handle;
}

entt::handle scan_category_data_t::get_assets_handle() const {
  auto l_prj_handle = get_project_handle();

  for (auto l_c : l_prj_handle.get<assets>().get_child()) {
    if (l_c && l_c.all_of<assets>() && l_c.get<assets>().p_path == file_type_.p_path) return l_c;
  }

  entt::handle l_handle{*g_reg(), g_reg()->create()};

  l_handle.emplace<assets>(file_type_.p_path);
  l_handle.emplace<database>();

  l_prj_handle.get<assets>().add_child(l_handle);
  return l_handle;
}

std::vector<entt::handle> scan_category_data_t::create_handles(
    const std::map<uuid, entt::handle>& in_handle_map, entt::registry& in_reg
) const {
  std::vector<entt::handle> l_out{};

  if (rig_file_.path_.empty()) return l_out;
  if (ue_file_.path_.empty()) return l_out;
  auto l_prj_handle = get_project_handle();
  auto l_ass_handle = get_assets_handle();

  entt::handle l_ue_handle{};
  if (in_handle_map.contains(ue_file_.uuid_)) {
    l_ue_handle = in_handle_map.at(ue_file_.uuid_);
  } else {
    l_ue_handle = l_out.emplace_back(in_reg, in_reg.create());
    l_ue_handle.emplace<assets_file>(ue_file_.path_, name_, 0).assets_attr(l_ass_handle);
    l_ue_handle.emplace<season>(season_);
    ue_main_map::find_ue_project_file(l_ue_handle);
    l_ue_handle.emplace<database>(ue_file_.uuid_);

    switch (assets_type_) {
      case assets_type_enum::scene:
        l_ue_handle.emplace<scene_id>();
        break;
      case assets_type_enum::prop:
        l_ue_handle.emplace<prop_id>();
        break;
      case assets_type_enum::character:
        l_ue_handle.emplace<character_id>();
        break;
      default:
        default_logger_raw()->log(log_loc(), level::err, "无法识别的类型:{}", magic_enum::enum_name(assets_type_));
        break;
    }
  }
  // 创建关联句柄
  entt::handle l_file_handle{};
  if (!l_ue_handle.any_of<file_association_ref>()) {
    l_file_handle = l_out.emplace_back(entt::handle{in_reg, in_reg.create()});
    l_file_handle.emplace<database>();
    l_file_handle.emplace<file_association>();
  } else {
    l_file_handle = l_ue_handle.get<file_association_ref>();
  }

  l_file_handle.patch<file_association>().ue_file = l_ue_handle;
  l_ue_handle.emplace_or_replace<file_association_ref>(l_file_handle);

  // 创建rig句柄
  entt::handle l_rig_handle{};
  if (in_handle_map.contains(rig_file_.uuid_)) {
    l_rig_handle = in_handle_map.at(rig_file_.uuid_);
  } else {
    l_rig_handle = l_out.emplace_back(in_reg, in_reg.create());
    l_rig_handle.emplace<assets_file>(rig_file_.path_, name_, 0).assets_attr(l_ass_handle);
    l_rig_handle.emplace<season>(season_);
    l_rig_handle.emplace<rig_id>();
    l_rig_handle.emplace<database>(rig_file_.uuid_);
  }
  // 添加关联
  l_file_handle.patch<file_association>().maya_rig_file = l_rig_handle;
  l_rig_handle.emplace_or_replace<file_association_ref>(l_file_handle);

  return l_out;
}

}  // namespace doodle::details