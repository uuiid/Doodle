//
// Created by TD on 25-6-29.
//
#pragma once

#include "linear_layer.h"
#include "positional_encoding.h"
#include "timestep_embedder.h"

#include <doodle_lib/core/global_function.h>

#include <Eigen/Dense>
#include <cstdint>
#include <memory>
#include <mutex>
#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>

namespace doodle::ai {

// Eigen 没有 MatrixX<bool> 的预定义 typedef
using MatrixXb = Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic>;

/// @brief Transformer 编码器主干（对应 Python TransformerEncoderBlock）
///
/// 组件:
/// - embed_text: Linear(llm_dim -> latent_dim)  — 文本嵌入
/// - input_linear: Linear(input_dim -> latent_dim)  — 运动输入嵌入
/// - output_linear: Linear(latent_dim -> output_dim)  — 输出投影
/// - linear_first_heading_angle: Linear(2 -> latent_dim)  — 初始朝向角嵌入（可选）
/// - sequence_pos_encoder: PositionalEncoding  — 非学习型位置编码
/// - embed_timestep: TimestepEmbedder  — 扩散时间步编码
/// - seqTransEncoder: ONNX 导出的 TransformerEncoder  — 主编码器
class TransformerEncoderBlock {
  // ---- Linear 层（npy 加载，Eigen 实现） ----
  LinearLayer embed_text_;
  LinearLayer input_linear_;
  LinearLayer output_linear_;
  LinearLayer linear_first_heading_angle_;

  // ---- 位置编码 ----
  PositionalEncoding sequence_pos_encoder_;

  // ---- 时间步编码 ----
  TimestepEmbedder embed_timestep_;

  // ---- ONNX 参数 ----
  std::int64_t latent_dim_{};
  std::int64_t num_text_tokens_{};
  bool use_text_mask_{};
  bool input_first_heading_angle_{};
  FSys::path model_dir_;

  // ---- ONNX Runtime ----
  std::unique_ptr<Ort::Session> session_;
  std::unique_ptr<Ort::IoBinding> io_binding_;
  std::vector<std::string> input_names_;
  std::vector<std::string> output_names_;
  Ort::MemoryInfo memory_info_{Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)};
  std::once_flag session_init_flag_;

  void init_session();

 public:
  TransformerEncoderBlock() = default;

  /// @brief 加载所有权重并初始化 ONNX session
  /// @param model_dir 模型目录，包含以下文件:
  ///   - seq_trans_encoder.onnx  (ONNX 编码器)
  ///   - embed_text_weight.npy, embed_text_bias.npy
  ///   - input_linear_weight.npy, input_linear_bias.npy
  ///   - output_linear_weight.npy, output_linear_bias.npy
  ///   - linear_first_heading_angle_weight.npy, linear_first_heading_angle_bias.npy (可选)
  ///   - timestep_linear1_weight.npy, timestep_linear1_bias.npy
  ///   - timestep_linear2_weight.npy, timestep_linear2_bias.npy
  /// @param latent_dim 潜在空间维度
  /// @param num_text_tokens 最大文本 token 数
  /// @param use_text_mask 是否使用文本 mask
  /// @param input_first_heading_angle 是否输入初始朝向角
  void load(
      const FSys::path& model_dir,
      std::int64_t latent_dim,
      std::int64_t num_text_tokens,
      bool use_text_mask,
      bool input_first_heading_angle = false
  );

  /// @brief 正向传播（对应 Python forward）
  /// @param x [B, T, input_dim] 当前噪声运动（平坦化为 [B*T, input_dim]）
  /// @param x_pad_mask [B, T] 运动序列 mask（true=有效，false=填充）
  /// @param text_feat [B, max_text_len, llm_dim] 文本嵌入（平坦化为 [B*max_text_len, llm_dim]）
  /// @param text_feat_pad_mask [B, max_text_len] 文本 mask（true=有效，false=填充）
  /// @param timesteps [B] 当前扩散步
  /// @param first_heading_angle [B] 初始朝向角（可选，弧度）
  /// @return [B, T, output_dim] 去噪后的运动（平坦化为 [B*T, output_dim]）
  Eigen::MatrixXf forward(
      const Eigen::MatrixXf& x,
      const MatrixXb& x_pad_mask,
      const Eigen::MatrixXf& text_feat,
      const MatrixXb& text_feat_pad_mask,
      const std::vector<std::int64_t>& timesteps,
      const std::vector<float>& first_heading_angle = {}
  );
};

}  // namespace doodle::ai
