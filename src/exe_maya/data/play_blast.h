
//
// Created by TD on 2021/11/22.
//

#pragma once
#include "doodle_core/metadata/episodes.h"
#include "doodle_core/metadata/shot.h"

#include "doodle_lib/doodle_lib_fwd.h"

#include <maya_plug/main/maya_plug_fwd.h>

#include "maya/MApiNamespace.h"
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTime.h>
namespace doodle::maya_plug {

class play_blast {
 private:
  FSys::path p_save_path;
  MString p_camera_path;

  episodes p_eps;
  shot p_shot;

  MTime p_current_time;
  std::string p_uuid;

  static MString p_post_render_notification_name;
  static MString k_play_blast_tex;
  static void captureCallback(MHWRender::MDrawContext& context, void* clientData);

  FSys::path get_file_path(const MTime& in_time) const;
  FSys::path get_file_path() const;
  FSys::path get_file_dir() const;

  void play_blast_by_render(const MTime& in_start, const MTime& in_end) const;

 public:
  play_blast();

  MStatus play_blast_(const MTime& in_start, const MTime& in_end);

  bool conjecture_camera();
  bool conjecture_ep_sc();

  FSys::path set_save_path(const FSys::path& in_save_path);
  FSys::path set_save_dir(const FSys::path& in_save_dir);
  FSys::path set_save_filename(const FSys::path& in_save_filename);

  FSys::path get_out_path() const;
};

}  // namespace doodle::maya_plug
