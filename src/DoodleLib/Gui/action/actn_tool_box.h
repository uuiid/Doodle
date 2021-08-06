//
// Created by TD on 2021/8/4.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include "action.h"

namespace doodle {
namespace action_arg {
class DOODLELIB_API arg_tool_box_create_ue_shot : public arg_path {
 public:
  arg_tool_box_create_ue_shot();
  std::vector<ShotPtr> shot_list;
};
}  // namespace action_arg
namespace toolbox {
/**
 * @brief 这个是导出maya fbx 的动作， 需要一个多个maya文件输入
 *
 */
class DOODLELIB_API actn_export_maya
    : public action_toolbox<action_arg::arg_paths> {
 public:
  actn_export_maya();
  bool is_async() override;
  using arg = action_arg::arg_paths;

 protected:
  long_term_ptr run() override;
};

class DOODLELIB_API actn_create_video
    : public action_toolbox<action_arg::arg_paths> {
  std::vector<ImageSequencePtr> p_image;

 public:
  actn_create_video();
  bool is_async() override;
  using arg = action_arg::arg_paths;

 protected:
  long_term_ptr run() override;
};

class DOODLELIB_API actn_connect_video
    : public action_toolbox<action_arg::arg_paths> {
 public:
  actn_connect_video();
  bool is_async() override;
  using arg = action_arg::arg_paths;

 protected:
  long_term_ptr run() override;
};

class DOODLELIB_API actn_ue4_shot_episodes
    : public action_toolbox<action_arg::arg_tool_box_create_ue_shot> {
 public:
  actn_ue4_shot_episodes();
  bool is_async() override;
  using arg = action_arg::arg_tool_box_create_ue_shot;

 protected:
  long_term_ptr run() override;
};
}  // namespace toolbox
}  // namespace doodle
