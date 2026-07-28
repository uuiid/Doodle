//
// Created by TD on 25-7-17.
//
#pragma once

#include <doodle_lib/core/global_function.h>

#include "motion_rep/motion_rep_base.h"
#include "transformer_encoder_block.h"
#include <Eigen/Dense>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>


namespace doodle::ai {

/// @brief 两阶段去噪器：先预测全局根节点运动，再基于局部根节点预测身体运动（对应 Python TwostageDenoiser）
///
/// 架构:
///   Stage 1 (root_model_) : 从含噪运动预测全局根节点轨迹 [B, T, global_root_dim]
///   Stage 2 (body_model_) : 将全局根节点转为局部根节点，与原始身体拼接后预测身体运动 [B, T, input_dim -
///   global_root_dim] 输出: [B, T, input_dim] = 全局根节点预测 + 身体预测 拼接
///
/// motion_mask_mode 支持:
///   - "none" : 无运动 mask
///   - "concat" : motion_mask 与 x 拼接作为额外通道输入
class twostage_denoiser {
  // ---- 运动表示维度 ----
  std::int64_t global_root_dim_{};
  std::int64_t local_root_dim_{};
  std::int64_t input_dim_{};

  // ---- 配置 ----
  std::string motion_mask_mode_{"none"};

  // ---- 运动表示（弱指针，防止循环引用） ----
  std::weak_ptr<motion_rep_base> motion_rep_;

  // ---- 两个 Transformer 编码器 ----
  transformer_encoder_block root_model_;
  transformer_encoder_block body_model_;

 public:
  twostage_denoiser() = default;

  /// @brief 加载所有权重并初始化两个 transformer_encoder_block
  /// @param root_model_dir 根节点模型目录
  /// @param body_model_dir 身体模型目录
  /// @param latent_dim Transformer 潜在空间维度
  /// @param num_text_tokens 最大文本 token 数
  /// @param use_text_mask 是否使用文本 mask
  /// @param motion_rep 运动表示（从中推导 input_dim / global_root_dim / local_root_dim）
  /// @param motion_mask_mode 运动 mask 模式 ("none" 或 "concat")
  /// @param input_first_heading_angle 是否输入初始朝向角
  void load(
      const FSys::path& root_model_dir, const FSys::path& body_model_dir, std::int64_t latent_dim,
      std::int64_t num_text_tokens, bool use_text_mask, const std::shared_ptr<motion_rep_base>& motion_rep,
      const std::string& motion_mask_mode = "none", bool input_first_heading_angle = false
  );

  /// @brief 正向传播（对应 Python forward）
  /// @param x [B*T, input_dim] 当前噪声运动（平坦化）
  /// @param x_pad_mask [B, T] 运动序列 mask（true=有效，false=填充）
  /// @param text_feat [B*max_text_len, llm_dim] 文本嵌入（平坦化）
  /// @param text_feat_pad_mask [B, max_text_len] 文本 mask（true=有效，false=填充）
  /// @param timesteps [B] 当前扩散步
  /// @param first_heading_angle [B] 初始朝向角（可选，弧度）
  /// @param motion_mask [B*T, input_dim] 运动 mask（可选，concat 模式使用）
  /// @param observed_motion [B*T, input_dim] 观测运动（可选，concat 模式使用）
  /// @return [B*T, input_dim] 去噪后的运动（平坦化）
  Eigen::MatrixXf forward(
      const Eigen::MatrixXf& x, const MatrixXb& x_pad_mask, const Eigen::MatrixXf& text_feat,
      const MatrixXb& text_feat_pad_mask, const std::vector<std::int64_t>& timesteps,
      const std::vector<float>& first_heading_angle = {}, const Eigen::MatrixXf& motion_mask = {},
      const Eigen::MatrixXf& observed_motion = {}
  );

  [[nodiscard]] bool is_valid() const { return input_dim_ > 0 && global_root_dim_ > 0; }
  [[nodiscard]] std::int64_t global_root_dim() const { return global_root_dim_; }
  [[nodiscard]] std::int64_t local_root_dim() const { return local_root_dim_; }
  [[nodiscard]] std::int64_t input_dim() const { return input_dim_; }
  [[nodiscard]] std::int64_t body_dim() const { return input_dim_ - global_root_dim_; }
};

}  // namespace doodle::ai
