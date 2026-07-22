//
// Created by TD on 25-7-21.
//
#pragma once

#include <doodle_lib/ai/skeleton/skeleton_base.h>
#include <doodle_lib/core/global_function.h>

#include <Eigen/Dense>
#include <cstdint>
#include <vector>

namespace doodle::ai {

// ======================================================================
// 前向运动学（FK）
// ======================================================================

/// @brief 前向运动学：由局部旋转和根位置计算全局旋转和关节位置
///
/// @param local_rot_mats [B*T, J*9] 局部旋转矩阵（每行 9 个元素 = 3x3 矩阵按行展开）
/// @param root_positions [B*T, 3] 根关节世界坐标
/// @param skel 骨骼定义
/// @return (global_rot_mats [B*T, J*9], posed_joints [B*T, J*3], posed_joints_norootpos [B*T, J*3])
struct fk_result {
  Eigen::MatrixXf global_rot_mats;        ///< [B*T, J*9] 全局旋转矩阵
  Eigen::MatrixXf posed_joints;           ///< [B*T, J*3] 全局关节位置
  Eigen::MatrixXf posed_joints_norootpos; ///< [B*T, J*3] 无根偏移的关节位置
};

fk_result fk(
    const Eigen::MatrixXf& local_rot_mats,
    const Eigen::MatrixXf& root_positions,
    const skeleton_base& skel
);

// ======================================================================
// 全局旋转 → 局部旋转
// ======================================================================

/// @brief 由全局旋转矩阵计算局部旋转（逆父级变换）
/// @param global_rot_mats [B*T, J*9] 全局旋转
/// @param skel 骨骼定义
/// @return [B*T, J*9] 局部旋转
Eigen::MatrixXf global_rots_to_local_rots(
    const Eigen::MatrixXf& global_rot_mats,
    const skeleton_base& skel
);

}  // namespace doodle::ai
