
//
// Created by TD on 2021/11/22.
//

#pragma once
#include "doodle_core/metadata/episodes.h"
#include "doodle_core/metadata/image_size.h"
#include "doodle_core/metadata/shot.h"


#include <maya_plug/main/maya_plug_fwd.h>

#include "maya/MApiNamespace.h"
#include <filesystem>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTime.h>
namespace doodle::maya_plug {

class play_blast {
 private:
  MString p_camera_path;

  MTime p_current_time;
  std::string p_uuid;

  static MString p_post_render_notification_name;
  static MString k_play_blast_tex;
  static void captureCallback(MHWRender::MDrawContext& context, void* clientData);

  FSys::path get_file_path(const MTime& in_time) const;
  FSys::path get_file_dir() const;

 public:
  play_blast();

  FSys::path play_blast_(const MTime& in_start, const MTime& in_end, const image_size& in_size);

  inline FSys::path get_out_path() const { return get_file_dir(); }
};

}  // namespace doodle::maya_plug
