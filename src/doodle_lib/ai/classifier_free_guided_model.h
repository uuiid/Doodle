//
// Created by TD on 25-7-20.
//
#pragma once

#include <doodle_lib/ai/cfg_type.h>
#include <doodle_lib/ai/fwd.h>
#include <doodle_lib/ai/motion_rep/motion_rep_base.h>
#include <doodle_lib/ai/twostage_denoiser.h>
#include <doodle_lib/core/global_function.h>

#include <Eigen/Dense>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>


namespace doodle::ai {

struct kimodo_model_config;

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

 public:
  classifier_free_guided_model() = default;

  /// @brief 加载去噪器模型
  /// @param config 模型配置（共享指针），提供路径、维度、标志等参数
  /// @param motion_rep 运动表示
  void load(std::shared_ptr<kimodo_model_config> config, const std::shared_ptr<motion_rep_base>& motion_rep);

  void load_onnx() { model_.load_onnx(); }
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
  /// @param cfg_type_val CFG 类型
  /// @return [B*T, input_dim] 去噪后的运动（平坦化）
  MatrixXfRow forward(
      const std::vector<float>& cfg_weight, const MatrixXfRow& x, const MatrixXbRow& x_pad_mask,
      const MatrixXfRow& text_feat, const MatrixXbRow& text_feat_pad_mask, const std::vector<std::int64_t>& timesteps,
      const std::vector<float>& first_heading_angle = {}, const MatrixXfRow& motion_mask = {},
      const MatrixXfRow& observed_motion = {}, cfg_type cfg_type_val = cfg_type::nocfg
  );

  [[nodiscard]] bool is_valid() const { return model_.is_valid(); }
  [[nodiscard]] const twostage_denoiser& model() const { return model_; }
  [[nodiscard]] twostage_denoiser& model() { return model_; }
};

}  // namespace doodle::ai
