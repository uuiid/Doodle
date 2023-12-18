//
// Created by TD on 2023/12/15.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <entt/entt.hpp>
#include <vector>
namespace doodle {
/**
 * 这个文件的路径储存在 asset_file 中
 * 一个 maya 文件可能会有多个 maya_rig_file,
 * 不一定每个 rig 都会有 sim_file
 */

class maya_anim_file {
 public:
  struct maya_ref_and_path {
    FSys::path path{};
    entt::handle ref{};
  };
  /// 开始帧
  std::int32_t begin_frame_{};
  /// 结束帧
  std::int32_t end_frame_{};
  /// maya引用文件,  非特指场景
  std::vector<maya_ref_and_path> maya_rig_file_{};

  FSys::path camera_path{};
};

}  // namespace doodle