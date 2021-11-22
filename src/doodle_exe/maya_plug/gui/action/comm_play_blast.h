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

class camera_filter {
  bool chick_cam(MDagPath& in_path);

  class regex_priority_pair {
   public:
    std::regex reg;
    std::int32_t priority;
  };

 public:
  class camera {
   public:
    MObject p_dag_path;
    std::int32_t priority;
    bool operator<(const camera& in_rhs) const;
    bool operator>(const camera& in_rhs) const;
    bool operator<=(const camera& in_rhs) const;
    bool operator>=(const camera& in_rhs) const;
  };

  std::vector<camera> p_list;

  camera_filter();
  /**
   * @brief 获得最高优先级的cam
   * @warning 可能为空obj
   *
   * @return MObject
   */
  MObject get() const;

  bool conjecture();
};

class comm_play_blast : public command_base {
  bool use_conjecture_cam;
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
  FSys::path get_out_path() const;


  bool conjecture_camera();
 public:
  comm_play_blast();

  MStatus play_blast(const MTime& in_start, const MTime& in_end);
  bool conjecture_ep_sc();
  bool render() override;
};
}  // namespace doodle::maya_plug
