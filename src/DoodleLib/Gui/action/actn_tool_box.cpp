//
// Created by TD on 2021/8/4.
//

#include "actn_tool_box.h"

#include <threadPool/long_term.h>

namespace doodle::toolbox {

actn_export_maya::actn_export_maya()
    : action_toolbox<action_arg::arg_paths>() {
  p_term = std::make_shared<long_term>();
  p_name = "导出fbx";
}
long_term_ptr actn_export_maya::run() {
  p_date = sig_get_arg().value();

  if (p_date.is_cancel) {
    cancel("取消导出");
    return p_term;
  }

  return p_term;
}
bool actn_export_maya::is_async() {
  return true;
}

actn_create_video::actn_create_video()
    : action_toolbox<action_arg::arg_paths>() {
  p_term = std::make_shared<long_term>();
  p_name = "创建视频";
}
bool actn_create_video::is_async() {
  return true;
}
long_term_ptr actn_create_video::run() {
  return p_term;
}

actn_connect_video::actn_connect_video()
    : action_toolbox<action_arg::arg_paths>() {
  p_term = std::make_shared<long_term>();
  p_name = "连接视频";
}
bool actn_connect_video::is_async() {
  return true;
}
long_term_ptr actn_connect_video::run() {
  return p_term;
}

actn_ue4_shot_episodes::actn_ue4_shot_episodes()
    : action_toolbox<action_arg::arg_path>() {
  p_term = std::make_shared<long_term>();
  p_name = "创建ue4关卡";
}
bool actn_ue4_shot_episodes::is_async() {
  return true;
}
long_term_ptr actn_ue4_shot_episodes::run() {
  return p_term;
}
}  // namespace doodle::toolbox
