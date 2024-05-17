//
// Created by TD on 2024/3/22.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <argh.h>
namespace doodle::launch {
class auto_light_process_t {
  static constexpr auto g_animation        = "animation";
  static constexpr auto g_cfx              = "cfx";
  static constexpr auto g_maya_file        = "maya_file";
  static constexpr auto g_export_anim_time = "export_anim_time";
  static constexpr auto g_only_map_drive   = "only_map_drive";

 public:
  auto_light_process_t()  = default;
  ~auto_light_process_t() = default;

  bool operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector);
};
}  // namespace doodle::launch
