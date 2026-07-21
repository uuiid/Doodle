//
// Created by TD on 25-7-21.
//
#pragma once

#include <doodle_lib/core/global_function.h>

#include <Eigen/Dense>
#include <cstdint>
#include <vector>

namespace doodle::ai {

/// @brief 轻量骨骼数据结构（对应 Python SkeletonBase）
///
/// 存储运动学（FK）所需的关节层级、中性姿态和语义标签。
/// 由具体骨骼工厂函数（如 make_soma_skeleton_30()）构造。
struct skeleton {
  std::int64_t nbjoints{};            ///< 关节数量
  std::int64_t root_idx{};            ///< 根关节索引（通常为髋部）

  std::vector<std::int64_t> joint_parents;  ///< [J] 父关节索引，根关节为 -1
  Eigen::MatrixXf neutral_joints;           ///< [J, 3] 中性姿态关节位置

  // --- 语义标签 ---
  std::vector<std::int64_t> left_foot_joint_idx;   ///< 左脚关节索引（链顺序）
  std::vector<std::int64_t> right_foot_joint_idx;  ///< 右脚关节索引（链顺序）
  std::vector<std::int64_t> hip_joint_idx;          ///< [right, left] 髋关节索引

  // --- 层级深度分组（用于 FK 逐级更新） ---
  std::vector<std::vector<std::int64_t>> joint_levels;

  /// @brief 从 joint_parents 构建 joint_levels（层级深度分组）
  void build_joint_levels();
};

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
    const skeleton& skel
);

/// @brief 批量刚体变换
/// @param rot_mats [B, J, 3, 3] 局部旋转矩阵（已平坦化为 [B*J, 9] 或类似形式）
/// @param joints [B, J, 3] 初始关节位置
/// @param parents [J] 父关节索引
/// @param root_idx 根关节索引
/// @return (posed_joints [B, J, 3], global_rot_mats [B, J, 3, 3])
/// 注意：输入输出均为平坦化格式
struct batch_rigid_transform_result {
  Eigen::MatrixXf posed_joints;      ///< [B*J, 3]
  Eigen::MatrixXf global_rot_mats;   ///< [B*J, 9]
};

// ======================================================================
// 全局旋转 → 局部旋转
// ======================================================================

/// @brief 由全局旋转矩阵计算局部旋转（逆父级变换）
/// @param global_rot_mats [B*T, J*9] 全局旋转
/// @param skel 骨骼定义
/// @return [B*T, J*9] 局部旋转
Eigen::MatrixXf global_rots_to_local_rots(
    const Eigen::MatrixXf& global_rot_mats,
    const skeleton& skel
);

}  // namespace doodle::ai
