//
// Created by TD on 2023/12/20.
//
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/file_association.h>

#include <doodle_lib/gui/widgets/scan_assets/base.h>

namespace doodle::gui::details {

std::vector<entt::handle> scan_category_data_t::create_handles(
    const std::map<uuid, entt::handle>& in_handle_map, entt::registry& in_reg
) const {
  std::vector<entt::handle> l_out{};
  if (ue_file_.path_.empty()) return l_out;
  entt::handle l_ue_handle{};
  if (in_handle_map.contains(ue_file_.uuid_)) {
    l_ue_handle = l_out.emplace_back(in_handle_map.at(ue_file_.uuid_));
  } else {
    l_ue_handle = l_out.emplace_back(in_reg, in_reg.create());
    l_ue_handle.emplace<assets_file>(ue_file_.path_, name_, 0);
    l_ue_handle.emplace<season>(season_);
  }
  // 创建关联句柄
  entt::handle l_file_handle{};
  if (!l_ue_handle.any_of<file_association_ref>()) {
    l_file_handle                                     = entt::handle{in_reg, in_reg.create()};
    l_file_handle.emplace<file_association>().ue_file = l_ue_handle;

    l_ue_handle.emplace<file_association_ref>(l_file_handle);
  } else {
    l_file_handle = l_ue_handle.get<file_association_ref>();
  }
  // 创建rig句柄
  if (!rig_file_.path_.empty()) {
    entt::handle l_rig_handle{};
    if (in_handle_map.contains(rig_file_.uuid_)) {
      l_rig_handle = l_out.emplace_back(in_handle_map.at(rig_file_.uuid_));
    } else {
      l_rig_handle = l_out.emplace_back(in_reg, in_reg.create());
      l_rig_handle.emplace<assets_file>(rig_file_.path_, name_, 0);
      l_rig_handle.emplace<season>(season_);
    }
    // 添加关联
    l_file_handle.emplace<file_association>().maya_rig_file = l_rig_handle;
    l_rig_handle.emplace<file_association_ref>(l_file_handle);
  }
  return l_out;
}

}  // namespace doodle::gui::details