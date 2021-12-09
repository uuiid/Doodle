//
// Created by TD on 2021/11/16.
//

#pragma once

#include <doodle_lib/gui/action/command.h>
#include <maya_plug/maya_plug_fwd.h>

#include <maya/MTemplateCommand.h>
namespace doodle::maya_plug {

class comm_play_blast : public command_base {
  bool use_conjecture_cam;
  string p_save_path;
  MString p_camera_path;

  play_blast_ptr p_play_balst;

 public:
  comm_play_blast();

  bool render() override;
};

MSyntax comm_play_blast_maya_syntax();
constexpr char comm_play_blast_maya_name[] = "comm_play_blast_maya";
class comm_play_blast_maya : public MTemplateAction<
                                 comm_play_blast_maya,
                                 comm_play_blast_maya_name,
                                 comm_play_blast_maya_syntax> {
 public:
  MStatus doIt(const MArgList& in_arg) override;
};

}  // namespace doodle::maya_plug
