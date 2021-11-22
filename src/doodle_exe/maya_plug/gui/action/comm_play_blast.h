//
// Created by TD on 2021/11/16.
//

#pragma once

#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/metadata/episodes.h>
#include <doodle_lib/metadata/shot.h>
#include <maya/MPxCommand.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya_plug/maya_plug_fwd.h>
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

class comm_play_blast_maya : public MPxCommand {
 public:
  static MString comm_name;
  MStatus doIt(const MArgList& in_arg) override;
  static void* creator();
};

}  // namespace doodle::maya_plug
