//
// Created by TD on 25-7-20.
//
#pragma once

#include "motion_rep/motion_rep_base.h"
#include "twostage_denoiser.h"

#include <doodle_lib/core/global_function.h>

#include <Eigen/Dense>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace doodle::ai {

/// @brief 无分类器引导包装器（对应 Python ClassifierFreeGuidedModel）
///
/// 在采样时包装去噪器，实现无分类器引导（Classifier-Free Guidance, CFG）。
///
/// 支持的 cfg_type:
///   - "nocfg": 无引导，直接调用 model.forward
///   - "regular": out_uncond + w * (out_cond - out_uncond)
///     将条件和无条件输入拼接后 batch 推理
///   - "separated": out_uncond + w_text * (out_text - out_uncond) +
///                   w_constraint * (out_constraint - out_uncond)
///     将文本条件、约束条件、无条件三份拼接后 batch 推理
class classifier_free_guided_model {
  /// 被包装的去噪器模型
  twostage_denoiser model_;
  /// 默认 CFG 类型
  std::string cfg_type_default_{"separated"};

 public:
  classifier_free_guided_model() = default;

  /// @brief 加载去噪器模型
  void load(
      const FSys::path& root_model_dir,
      const FSys::path& body_model_dir,
      std::int64_t latent_dim,
      std::int64_t num_text_tokens,
      bool use_text_mask,
      const std::shared_ptr<motion_rep_base>& motion_rep,
      const std::string& motion_mask_mode   = "none",
      bool input_first_heading_angle         = false
  );

  /// @brief 设置默认 cfg_type
  void set_cfg_type_default(const std::string& cfg_type) { cfg_type_default_ = cfg_type; }

  /// @brief 带 CFG 的正向传播（对应 Python ClassifierFreeGuidedModel.forward）
  ///
  /// 根据 cfg_type 对输入沿 batch 维度拼接，一次 forward 同时计算条件和无条件输出，
  /// 然后按 CFG 公式组合。
  ///
  /// @param cfg_weight 引导权重:
  ///   - regular 模式: 单个 float 的 vector {w}
  ///   - separated 模式: 两个 float 的 vector {w_text, w_constraint}
  /// @param x [B*T, input_dim] 当前噪声运动（平坦化）
  /// @param x_pad_mask [B, T] 运动序列 mask（true=有效）
  /// @param text_feat [B*max_text_len, llm_dim] 文本嵌入（平坦化）
  /// @param text_feat_pad_mask [B, max_text_len] 文本 mask（true=有效）
  /// @param timesteps [B] 当前扩散步
  /// @param first_heading_angle [B] 初始朝向角（可选，弧度）
  /// @param motion_mask [B*T, input_dim] 运动 mask（可选）
  /// @param observed_motion [B*T, input_dim] 观测运动（可选）
  /// @param cfg_type CFG 类型；为空时使用 cfg_type_default_
  /// @return [B*T, input_dim] 去噪后的运动（平坦化）
  Eigen::MatrixXf forward(
      const std::vector<float>& cfg_weight,
      const Eigen::MatrixXf& x,
      const MatrixXb& x_pad_mask,
      const Eigen::MatrixXf& text_feat,
      const MatrixXb& text_feat_pad_mask,
      const std::vector<std::int64_t>& timesteps,
      const std::vector<float>& first_heading_angle  = {},
      const Eigen::MatrixXf& motion_mask              = {},
      const Eigen::MatrixXf& observed_motion          = {},
      const std::string& cfg_type                     = ""
  );

  [[nodiscard]] bool is_valid() const { return model_.is_valid(); }
  [[nodiscard]] const twostage_denoiser& model() const { return model_; }
  [[nodiscard]] twostage_denoiser& model() { return model_; }
  [[nodiscard]] const std::string& cfg_type_default() const { return cfg_type_default_; }
};

}  // namespace doodle::ai
