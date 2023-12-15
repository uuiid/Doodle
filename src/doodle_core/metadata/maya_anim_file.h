//
// Created by TD on 2023/12/15.
//

#pragma once
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
  /// 开始帧
  std::int32_t begin_frame_{};
  /// 结束帧
  std::int32_t end_frame_{};
  /// maya绑定文件, 特指人物
  std::vector<entt::handle> maya_rig_file_{};
  /// 场景只有一个
  entt::handle maya_scene_file_{};
};

}  // namespace doodle