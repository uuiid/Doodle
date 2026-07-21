//
// Created by TD on 25-7-21.
//
#pragma once

#include <Eigen/Dense>
#include <cstdint>

namespace doodle::ai {

/// @brief 将旋转矩阵转换为 6D 连续表示（前两列）
/// @param matrix [N, 9] 每行为 3x3 矩阵的行展开
/// @return [N, 6] 6D 连续表示
Eigen::MatrixXf matrix_to_cont6d(const Eigen::MatrixXf& matrix);

/// @brief 将 6D 连续表示恢复为旋转矩阵（Gram–Schmidt 正交化）
/// @param cont6d [N, 6] 6D 连续表示
/// @return [N, 9] 旋转矩阵行展开
Eigen::MatrixXf cont6d_to_matrix(const Eigen::MatrixXf& cont6d);

/// @brief 构建绕 Y 轴的旋转矩阵
/// @param angle 旋转角（弧度）
/// @return [3, 3] 绕 Y 轴旋转矩阵
Eigen::Matrix3f angle_to_Y_rotation_matrix(float angle);

/// @brief 批量构建绕 Y 轴的旋转矩阵
/// @param angles [N] 旋转角数组（弧度）
/// @return [N, 9] 每行为 3x3 矩阵的行展开
Eigen::MatrixXf angle_to_Y_rotation_matrix_batch(const Eigen::VectorXf& angles);

}  // namespace doodle::ai
