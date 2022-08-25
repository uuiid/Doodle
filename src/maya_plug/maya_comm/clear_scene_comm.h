//
// Created by TD on 2022/2/28.
//
#pragma once

#include <main/maya_plug_fwd.h>
namespace doodle::maya_plug {
namespace {
constexpr char doodle_clear_scene[] = "doodle_clear_scene";
}
MSyntax clear_scene_comm_syntax();
class clear_scene_comm : public TemplateAction<clear_scene_comm, doodle_clear_scene, clear_scene_comm_syntax> {
 public:
  MStatus doIt(const MArgList& in_arg) override;

  static bool show_save_mag();
};

}  // namespace doodle::maya_plug
