//
// Created by TD on 2022/2/28.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>
namespace doodle::maya_plug {
class maya_clear_scenes {
 public:
  maya_clear_scenes();
  static bool unlock_normal();
  static bool duplicate_name(MSelectionList& in_select);
  static bool multilateral_surface(MSelectionList& in_select);
  static std::tuple<bool, MSelectionList> multilateral_surface_by_select(const MSelectionList& in_select);
  static bool uv_set(MSelectionList& in_select);

  static bool err_1();  // (1)大纲
  static bool err_2();  // (2)onModelChange3dc
  static bool err_3();  // (3)CgAbBlastPanelOptChangeCallback
  static bool err_4();  // (4)贼健康

  static void delete_unknown_node();
};
}  // namespace doodle::maya_plug
