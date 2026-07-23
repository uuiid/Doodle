//
// Created by TD on 25-7-23.
//
#pragma once

#include "classifier_free_guided_model.h"
#include "diffusion.h"
#include "llm2vec.h"  // doodle::http::LLM2Vec
#include "motion_rep/kimodo_motion_rep.h"
#include "skeleton/skeleton_base.h"

#include <doodle_lib/core/global_function.h>

#include <Eigen/Dense>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace doodle::ai {

/// @brief Kimodo 主编排类（对应 Python Kimodo）
///
/// 编排完整推理管线: 文本编码 → 去噪循环 → 运动解码。
/// 子组件均为已实现的 C++ 类。
///
/// 用法:
/// @code
///   auto kmd = std::make_shared<kimodo>();
///   kmd->load(...);
///   auto output = kmd->generate({"a person walks"}, {120}, 50);
/// @endcode
///
/// @note _multiprompt（多段拼接）暂不实现。
class kimodo {
  // ---- 核心组件 ----
  classifier_free_guided_model denoiser_;  ///< CFG 包装的去噪器
  diffusion diffusion_;                     ///< 扩散过程
  ddim_sampler sampler_;                    ///< DDIM 采样器
  std::shared_ptr<kimodo_motion_rep> motion_rep_;  ///< 运动表示
  std::shared_ptr<skeleton_base> skeleton_;        ///< 骨骼 (用于输出转换)

  // ---- 文本编码器（在 doodle::http 命名空间） ----
  std::shared_ptr<doodle::http::LLM2Vec> text_encoder_;

  // ---- 配置 ----
  std::string cfg_type_default_{"separated"};
  float fps_{30.0f};
  std::int64_t llm_dim_{4096};  ///< 文本嵌入维度

  // ---- 内部辅助 ----
  struct text_encoding_result {
    Eigen::MatrixXf text_feat;    ///< [B, D] 文本嵌入（已平坦化，实际形状 [B*1, D]）
    MatrixXb text_pad_mask;       ///< [B, 1] 文本 mask
  };

  /// @brief 编码文本（对应 Python _generate 中的 self.text_encoder(texts)）
  text_encoding_result encode_texts(const std::vector<std::string>& texts);

  /// @brief 单步去噪（对应 Python denoising_step）
  /// @return [B*T, D] t-1 步的噪声运动（平坦化）
  Eigen::MatrixXf denoising_step(
      const Eigen::MatrixXf& motion,
      const MatrixXb& pad_mask,
      const Eigen::MatrixXf& text_feat,
      const MatrixXb& text_pad_mask,
      std::int64_t t,
      const std::vector<float>& first_heading_angle,
      const Eigen::MatrixXf& motion_mask,
      const Eigen::MatrixXf& observed_motion,
      std::int64_t num_denoising_steps,
      const std::vector<float>& cfg_weight,
      const std::string& cfg_type
  );

  /// @brief 完整去噪循环（对应 Python _generate）
  /// @return [B*T, D] 去噪后的运动（平坦化，已标准化）
  Eigen::MatrixXf generate_internal(
      const std::vector<std::string>& texts,
      std::int64_t max_frames,
      std::int64_t num_denoising_steps,
      const MatrixXb& pad_mask,
      const std::vector<float>& first_heading_angle,
      const Eigen::MatrixXf& motion_mask,
      const Eigen::MatrixXf& observed_motion,
      const std::vector<float>& cfg_weight,
      const std::string& cfg_type
  );

 public:
  kimodo() = default;
  ~kimodo() = default;

  // 禁止拷贝
  kimodo(const kimodo&) = delete;
  kimodo& operator=(const kimodo&) = delete;

  /// @brief 从 npy 权重目录加载模型
  ///
  /// @param denoiser_root_dir 根节点去噪器目录（含 root_model 和 body_model 子目录）
  /// @param denoiser_body_dir 身体模型目录
  /// @param text_encoder_model_path LLM2Vec ONNX 模型路径
  /// @param tokenizer_json_path tokenizer.json 路径
  /// @param skeleton_dir 骨骼数据目录（可选，供 SOMASkeleton30 加载 npy）
  /// @param stats_path 标准化统计目录（含 global_root/ local_root/ body/ 子目录）
  /// @param num_base_steps 扩散基础步数（默认 1000）
  /// @param latent_dim Transformer 潜在维度（默认 1024）
  /// @param num_text_tokens 文本 token 数（默认 50）
  /// @param use_text_mask 是否使用文本 mask
  /// @param cfg_type 默认 CFG 类型
  /// @param llm_dim 文本嵌入维度（默认 4096）
  /// @param fps 帧率（默认 30）
  void load(
      const FSys::path& denoiser_root_dir,
      const FSys::path& denoiser_body_dir,
      const FSys::path& text_encoder_model_path,
      const FSys::path& tokenizer_json_path,
      const FSys::path& skeleton_dir,
      const FSys::path& stats_path,
      std::int64_t num_base_steps             = 1000,
      std::int64_t latent_dim                 = 1024,
      std::int64_t num_text_tokens            = 50,
      bool use_text_mask                      = false,
      const std::string& cfg_type             = "separated",
      std::int64_t llm_dim                    = 4096,
      float fps                               = 30.0f
  );

  /// @brief 从文本生成运动（对应 Python __call__，不含 multi_prompt）
  ///
  /// @param prompts 文本提示（每个元素对应一个样本）
  /// @param num_frames 各样本帧数（长度需与 prompts 相同）
  /// @param num_denoising_steps 去噪步数
  /// @param cfg_weight CFG 权重: {w_text, w_constraint}
  /// @param first_heading_angle 初始朝向角（弧度，[B] 或空）
  /// @param motion_mask 运动 mask [B*T, D]（可选）
  /// @param observed_motion 观测运动 [B*T, D]（可选）
  /// @param cfg_type CFG 类型；空则使用默认
  /// @return 运动输出结构体（平坦化，[B*T, ...]）
  motion_output generate(
      const std::vector<std::string>& prompts,
      const std::vector<std::int64_t>& num_frames,
      std::int64_t num_denoising_steps,
      const std::vector<float>& cfg_weight         = {2.0f, 2.0f},
      const std::vector<float>& first_heading_angle = {},
      const Eigen::MatrixXf& motion_mask            = {},
      const Eigen::MatrixXf& observed_motion        = {},
      const std::string& cfg_type                   = ""
  );

  /// @brief 简单版本：单个提示、单样本
  motion_output generate(
      const std::string& prompt,
      std::int64_t num_frames,
      std::int64_t num_denoising_steps,
      const std::vector<float>& cfg_weight         = {2.0f, 2.0f},
      float first_heading_angle                     = 0.0f
  );

  [[nodiscard]] bool is_valid() const {
    return denoiser_.is_valid() && diffusion_.is_valid() && motion_rep_ != nullptr;
  }

  [[nodiscard]] const std::shared_ptr<kimodo_motion_rep>& motion_rep() const { return motion_rep_; }
  [[nodiscard]] const std::shared_ptr<skeleton_base>& skeleton() const { return skeleton_; }
  [[nodiscard]] float fps() const { return fps_; }
};

}  // namespace doodle::ai
