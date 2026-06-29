//
// Created by TD on 25-6-29.
//
#pragma once

#include "linear_layer.h"
#include "positional_encoding.h"

#include <Eigen/Dense>
#include <cstdint>
#include <vector>

namespace doodle::ai {

/// @brief 扩散时间步编码器（对应 Python TimestepEmbedder）
/// 将时间步索引编码为位置编码后，通过 MLP 映射到 latent 空间
/// 结构: PE -> Linear -> SiLU -> Linear
class TimestepEmbedder {
  std::int64_t latent_dim_{};
  const PositionalEncoding* sequence_pos_encoder_{};  // 非拥有引用
  LinearLayer linear1_;
  LinearLayer linear2_;

 public:
  TimestepEmbedder() = default;

  /// @brief 初始化时间步编码器
  /// @param latent_dim 潜在空间维度
  /// @param pos_encoder 位置编码器（外部持有）
  /// @param linear1_weight linear1 权重路径
  /// @param linear1_bias linear1 偏置路径
  /// @param linear2_weight linear2 权重路径
  /// @param linear2_bias linear2 偏置路径
  void init(
      std::int64_t latent_dim,
      const PositionalEncoding* pos_encoder,
      const FSys::path& linear1_weight,
      const FSys::path& linear1_bias,
      const FSys::path& linear2_weight,
      const FSys::path& linear2_bias
  );

  /// @brief 正向传播
  /// @param timesteps [B] 时间步索引（int64 值）
  /// @return [B, 1, latent_dim]
  Eigen::MatrixXf forward(const std::vector<std::int64_t>& timesteps) const;

  [[nodiscard]] bool is_valid() const { return latent_dim_ > 0 && sequence_pos_encoder_ != nullptr; }
};

}  // namespace doodle::ai
