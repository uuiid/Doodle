//
// Created by TD on 2025-7-12.
//

#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>

namespace doodle {

enum class server_task_info_type : std::int32_t {
  // 未知类型
  unknown    = -1,
  // 导出fbx任务
  export_fbx = 0,
  // 导出解算任务
  export_sim,
  // 自动灯光任务
  auto_light,
  // 合成视频
  merge_video,
  // 连接视频
  connect_video,
  // 检查maya文件
  check_maya,
  // 替换maya引用文件
  replace_maya_ref,
  // 创建骨骼
  create_rig_sk,
  // 同步文件
  project_sync,
  // 深度估计
  depth_estimation,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    server_task_info_type, {
                               {server_task_info_type::export_fbx, "export_fbx"},
                               {server_task_info_type::export_sim, "export_sim"},
                               {server_task_info_type::auto_light, "auto_light"},
                               {server_task_info_type::merge_video, "merge_video"},
                               {server_task_info_type::connect_video, "connect_video"},
                               {server_task_info_type::check_maya, "check_maya"},
                               {server_task_info_type::replace_maya_ref, "replace_maya_ref"},
                               {server_task_info_type::depth_estimation, "depth_estimation"},
                           }
);

}  // namespace doodle