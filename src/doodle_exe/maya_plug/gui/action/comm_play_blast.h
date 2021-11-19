//
// Created by TD on 2021/11/16.
//

#pragma once

#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/metadata/episodes.h>
#include <doodle_lib/metadata/shot.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTime.h>
namespace doodle::maya_plug {

class comm_play_blast : public command_base {
  bool use_override;
  string p_save_path;
  MString p_camera_path;

  episodes p_eps;
  shot p_shot;

  MTime p_current_time;
  string p_uuid;

  static string p_post_render_notification_name;

  static void captureCallback(MHWRender::MDrawContext& context, void* clientData);
  bool init();

  FSys::path get_file_path(const MTime& in_time);
  FSys::path get_file_dir();
 public:
  comm_play_blast();

  MStatus play_blast(const MTime& in_start, const MTime& in_end);
  bool render() override;
};
}  // namespace doodle::maya_plug