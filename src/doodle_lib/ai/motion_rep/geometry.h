//
// Created by TD on 25-7-21.
//
#pragma once

#include <doodle_lib/ai/fwd.h>

#include <Eigen/Dense>

namespace doodle::ai {

/// @brief 将旋转矩阵转换为 6D 连续表示（前两列），支持多关节拼接
/// @param matrix [N, J*9] 每行为 J 个关节的 3x3 矩阵行展开拼接
/// @return [N, J*6] 6D 连续表示
MatrixXfRow matrix_to_cont6d(const MatrixXfRow& matrix);

/// @brief 将 6D 连续表示恢复为旋转矩阵（Gram–Schmidt 正交化），支持多关节拼接
/// @param cont6d [N, J*6] 6D 连续表示
/// @return [N, J*9] 旋转矩阵行展开
MatrixXfRow cont6d_to_matrix(const MatrixXfRow& cont6d);

// ======================================================================
// Axis-Angle ↔ Rotation Matrix (for constraint save/load)
// ======================================================================

/// @brief 将轴角表示转换为旋转矩阵（Rodrigues 公式）
/// @param axis_angle [N, 3] 每行为轴角向量（角度 = 范数，轴 = 归一化方向）
/// @return [N, 9] 每行为 3x3 旋转矩阵的行展开
MatrixXfRow axis_angle_to_matrix(const MatrixXfRow& axis_angle);

/// @brief 将旋转矩阵转换为轴角表示（通过四元数，数值稳定）
/// @param matrix [N, 9] 每行为 3x3 旋转矩阵的行展开
/// @return [N, 3] 轴角向量
MatrixXfRow matrix_to_axis_angle(const MatrixXfRow& matrix);

/// @brief 将旋转矩阵转换为四元数 (w, x, y, z)
/// @param matrix [N, 9] 旋转矩阵行展开
/// @return [N, 4] 四元数 (w, x, y, z)
MatrixXfRow matrix_to_quaternion(const MatrixXfRow& matrix);

/// @brief 将四元数 (w, x, y, z) 转换为轴角
/// @param quat [N, 4] 四元数
/// @return [N, 3] 轴角向量
MatrixXfRow quaternion_to_axis_angle(const MatrixXfRow& quat);

}  // namespace doodle::ai
