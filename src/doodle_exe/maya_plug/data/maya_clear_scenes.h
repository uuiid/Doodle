//
// Created by TD on 2022/2/28.
//

#pragma once

#include <maya_plug/maya_plug_fwd.h>
namespace doodle::maya_plug {
class maya_clear_scenes {
 public:
  maya_clear_scenes();
  bool unlock_normal();
  bool duplicate_name(MSelectionList& in_select);
  bool multilateral_surface(MSelectionList& in_select);
  bool uv_set(MSelectionList& in_select);
  bool err_1();  // (1)大纲
  bool err_2();  // (2)onModelChange3dc
  bool err_3();  // (3)CgAbBlastPanelOptChangeCallback
  bool err_4();  // (4)贼健康
};
}  // namespace doodle::maya_plug
