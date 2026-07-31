//
// Created by TD on 25-7-21.
//
#pragma once
#include <doodle_lib/ai/fwd.h>
#include <doodle_lib/ai/skeleton/skeleton_base.h>

#include <Eigen/Eigen>
#include <cstdint>
#include <vector>

namespace doodle::ai {

// ======================================================================
// 角度计算
// ======================================================================

/// @brief 计算帧间角度差（对应 Python diff_angles）
/// @param angles [B, T] 角度序列（弧度）
/// @param fps 帧率
/// @return [B, T] 角度差（补零至原始长度）
MatrixXfRow diff_angles(const MatrixXfRow& angles, float fps);

/// @brief 计算头部朝向角（对应 Python compute_heading_angle）
/// @param posed_joints [B*T, J*3] 全局关节位置
/// @param skel 骨骼定义
/// @param batch_size B
/// @param time_steps T
/// @return [B, T] 各帧头部朝向角（弧度）
MatrixXfRow compute_heading_angle(
    const MatrixXfRow& posed_joints, const skeleton_base& skel, std::int64_t batch_size, std::int64_t time_steps
);

// ======================================================================
// 速度计算
// ======================================================================

/// @brief 计算关节速度（对应 Python compute_vel_xyz）
/// @param positions [B*T, J*3] 关节位置
/// @param fps 帧率
/// @param batch_size B
/// @param time_steps T
/// @param nbjoints J
/// @param lengths [B] 各样本有效帧数（用于 padding 处理）
/// @return [B*T, J*3] 速度
MatrixXfRow compute_vel_xyz(
    const MatrixXfRow& positions, float fps, std::int64_t batch_size, std::int64_t time_steps, std::int64_t nbjoints,
    const Eigen::VectorXi& lengths
);

/// @brief 计算局部根节点旋转速度（对应 Python compute_vel_angle）
/// @param root_rot_angles [B, T] 根节点朝向角
/// @param fps 帧率
/// @param lengths [B] 各样本有效帧数
/// @return [B, T] 局部根节点旋转速度
MatrixXfRow compute_vel_angle(const MatrixXfRow& root_rot_angles, float fps, const Eigen::VectorXi& lengths);

// ======================================================================
// 脚接触检测
// ======================================================================

/// @brief 基于关节高度和速度检测脚接触（对应 Python foot_detect_from_pos_and_vel）
/// @param positions [B*T, J*3] 全局关节位置
/// @param velocity [B*T, J*3] 关节速度
/// @param skel 骨骼定义
/// @param batch_size B
/// @param time_steps T
/// @param vel_thres 速度阈值
/// @param height_thresh 高度阈值
/// @return [B*T, 4] 脚接触标签 [左跟, 左趾, 右跟, 右趾]
MatrixXbRow foot_detect_from_pos_and_vel(
    const MatrixXfRow& positions, const MatrixXfRow& velocity, const skeleton_base& skel, std::int64_t batch_size,
    std::int64_t time_steps, float vel_thres = 0.15f, float height_thresh = 0.10f
);

// ======================================================================
// 长度 → 掩码
// ======================================================================

/// @brief 将序列长度转换为布尔掩码（对应 Python length_to_mask）
/// @param lengths [B] 各序列长度
/// @param max_len 最大长度（若 <=0 则使用 lengths 最大值）
/// @return [B, max_len] 布尔掩码，true=有效
MatrixXbRow length_to_mask(const Eigen::VectorXi& lengths, std::int64_t max_len = -1);

// ======================================================================
// 平滑根位置（简化版）
// ======================================================================

/// @brief 计算平滑根位置（对应 Python get_smooth_root_pos）
/// 使用移动平均平滑根轨迹
/// @param root_positions [B*T, 3] 根关节位置
/// @param batch_size B
/// @param time_steps T
/// @param window_size 平滑窗口大小
/// @return [B*T, 3] 平滑后的根位置
MatrixXfRow get_smooth_root_pos(
    const MatrixXfRow& root_positions, std::int64_t batch_size, std::int64_t time_steps, std::int64_t window_size = 5
);

// ======================================================================
// 旋转特征辅助类（对应 Python RotateFeatures）
// ======================================================================

class rotate_features {
 public:
  /// @brief 预计算一批角度对应的旋转矩阵
  /// @param angle [B] 旋转角（弧度）
  rotate_features(const Eigen::VectorXf& angle);

  /// @brief 旋转 3D 位置（绕 Y 轴）
  /// @param positions [B*T, 3]
  /// @param batch_size B
  /// @param time_steps T
  /// @return [B*T, 3]
  MatrixXfRow rotate_positions(const MatrixXfRow& positions, std::int64_t batch_size, std::int64_t time_steps) const;

  /// @brief 旋转 2D 位置（xz 平面）
  /// @param positions_2d [B*T, 2]
  /// @param batch_size B
  /// @param time_steps T
  /// @return [B*T, 2]
  MatrixXfRow rotate_2d_positions(
      const MatrixXfRow& positions_2d, std::int64_t batch_size, std::int64_t time_steps
  ) const;

  /// @brief 旋转 6D 旋转特征
  /// @param rotations_6d [B*T, J*6]
  /// @param batch_size B
  /// @param time_steps T
  /// @param nbjoints J
  /// @return [B*T, J*6]
  MatrixXfRow rotate_6d_rotations(
      const MatrixXfRow& rotations_6d, std::int64_t batch_size, std::int64_t time_steps, std::int64_t nbjoints
  ) const;

 private:
  MatrixXfRow corrective_mat_2d_T_;  ///< [B, 2, 2] 2D 旋转矩阵转置
  MatrixXfRow corrective_mat_Y_T_;   ///< < [B, 3, 3] 3D 绕 Y 轴旋转矩阵转置
};

}  // namespace doodle::ai
