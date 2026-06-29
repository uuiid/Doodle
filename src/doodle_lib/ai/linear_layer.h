//
// Created by TD on 25-6-29.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <Eigen/Dense>
#include <cstdint>
#include <vector>

namespace doodle::ai {

/// @brief 使用 Eigen 实现的线性层 (y = xW^T + b)
/// 权重从 .npy 文件加载（对应 PyTorch nn.Linear）
class LinearLayer {
  Eigen::MatrixXf weight_;  // [out_features, in_features]
  Eigen::VectorXf bias_;    // [out_features] 或空

 public:
  LinearLayer() = default;

  /// @brief 从 .npy 文件加载权重和偏置
  /// @param weight_path weight.npy 路径
  /// @param bias_path bias.npy 路径（若为空则不加载偏置）
  void load(const FSys::path& weight_path, const FSys::path& bias_path = {});

  /// @brief 设置权重矩阵（直接赋值）
  void set_weight(Eigen::MatrixXf weight) { weight_ = std::move(weight); }

  /// @brief 设置偏置向量
  void set_bias(Eigen::VectorXf bias) { bias_ = std::move(bias); }

  /// @brief 正向传播：对 2D 输入 [N, in_features] 应用线性变换
  /// @return [N, out_features]
  Eigen::MatrixXf forward(const Eigen::MatrixXf& input) const;

  /// @brief 正向传播：对 3D 输入 [B, T, in_features] 应用线性变换
  /// @param input 平坦化的输入 [B*T, in_features]
  /// @param batch_size B
  /// @param time_steps T
  /// @return 平坦化输出 [B*T, out_features]（调用方自行 reshape）
  Eigen::MatrixXf forward_batched(const Eigen::MatrixXf& input, std::int64_t batch_size, std::int64_t time_steps) const;

  [[nodiscard]] std::int64_t in_features() const { return weight_.cols(); }
  [[nodiscard]] std::int64_t out_features() const { return weight_.rows(); }
  [[nodiscard]] bool has_bias() const { return bias_.size() > 0; }
  [[nodiscard]] const Eigen::MatrixXf& weight() const { return weight_; }
  [[nodiscard]] const Eigen::VectorXf& bias() const { return bias_; }
  [[nodiscard]] bool is_valid() const { return weight_.size() > 0; }
};

}  // namespace doodle::ai
