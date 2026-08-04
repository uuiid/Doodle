//
// Created by TD on 26-8-4.
//
#pragma once

#include <doodle_lib/ai/fwd.h>
#include <doodle_lib/ai/motion_rep/constraint_set.h>
#include <doodle_lib/ai/skeleton/skeleton_base.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace doodle::ai {

// ======================================================================
// WorkingRigJoint — 工作骨骼关节描述符
// ======================================================================

/// @brief 工作骨骼关节（对应 Python SimpleNamespace 工作骨骼）
struct working_rig_joint {
  std::string name;
  std::string parent;                        ///< 父关节名，根关节为空
  std::array<float, 4> t_pose_rotation{};    ///< T-pose 局部旋转四元数 (w, x, y, z)
  std::array<float, 3> t_pose_translation{}; ///< T-pose 局部平移
  std::string retarget_tag;                  ///< 重定向标签 ("Hips", "LeftHand", ...)
};

// ======================================================================
// PostProcessResult
// ======================================================================

/// @brief post_process_motion 返回值
struct post_process_result {
  MatrixXfRow local_rot_mats;   ///< [B*T, J*9] 校正后的局部旋转矩阵
  MatrixXfRow root_positions;   ///< [B*T, 3] 校正后的根位置
  MatrixXfRow posed_joints;     ///< [B*T, J*3] 全局关节位置
  MatrixXfRow global_rot_mats;  ///< [B*T, J*9] 全局旋转矩阵
};

// ======================================================================
// create_working_rig_from_skeleton
// ======================================================================

/// @brief 从骨骼创建工作骨骼（对应 Python create_working_rig_from_skeleton）
/// @param skeleton 骨骼定义
/// @param above_ground_offset 离地偏移量（SOMA 默认 0.02，其他默认 0.007）
/// @return 工作骨骼关节列表
std::vector<working_rig_joint> create_working_rig_from_skeleton(
    const skeleton_base& skeleton, float above_ground_offset = 0.007f
);

// ======================================================================
// extract_input_motion_from_constraints
// ======================================================================

/// @brief 从约束中提取输入运动（髋部平移 + 局部旋转）
///        对应 Python extract_input_motion_from_constraints
/// @param constraint_lst 约束列表
/// @param skeleton 骨骼定义
/// @param num_frames 总帧数 T
/// @param num_joints 关节数 J
/// @return { hip_translations [T, 3], rotations [T, J*4] } 四元数 (w, x, y, z)
std::pair<MatrixXfRow, MatrixXfRow> extract_input_motion_from_constraints(
    const std::vector<constraint_set_ptr>& constraint_lst, const skeleton_base& skeleton, std::int64_t num_frames,
    std::int64_t num_joints
);

// ======================================================================
// post_process_motion
// ======================================================================

/// @brief 后处理生成的运动，减少滑步并提升质量
///        对应 Python post_process_motion
///
/// @param local_rot_mats  [B*T, J*9] 局部旋转矩阵（行展开）
/// @param root_positions  [B*T, 3] 根关节位置
/// @param contacts        [B*T, num_contacts] 脚接触标签
/// @param skeleton        骨骼定义
/// @param batch_size      批次大小 B
/// @param num_frames      帧数 T
/// @param constraint_lst  约束列表（所有批次共享，或为空）
/// @param batched_constraints 每个批次独立的约束列表（与 constraint_lst 二选一）
/// @param contact_threshold 脚接触检测阈值
/// @param root_margin     根位置校正边距
/// @return 校正后的运动数据
post_process_result post_process_motion(
    const MatrixXfRow& local_rot_mats, const MatrixXfRow& root_positions, const MatrixXfRow& contacts,
    const skeleton_base& skeleton, std::int64_t batch_size, std::int64_t num_frames,
    const std::vector<constraint_set_ptr>& constraint_lst = {},
    const std::vector<std::vector<constraint_set_ptr>>& batched_constraints = {}, float contact_threshold = 0.5f,
    float root_margin = 0.04f
);

}  // namespace doodle::ai