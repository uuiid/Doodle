//
// Created by TD on 25-7-23.
//
#pragma once

#include <doodle_lib/ai/classifier_free_guided_model.h>
#include <doodle_lib/ai/diffusion.h>
#include <doodle_lib/ai/fwd.h>
#include <doodle_lib/ai/llm2vec.h>
#include <doodle_lib/ai/motion_rep/kimodo_motion_rep.h>
#include <doodle_lib/ai/skeleton/skeleton_base.h>
#include <doodle_lib/core/global_function.h>

#include <Eigen/Dense>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace doodle::ai {

/// @brief Kimodo 模型配置（对应 model_config.json）
struct kimodo_model_config {
  std::string model_name_;
  std::string skeleton_type_;
  std::int32_t nb_joints_;
  std::int32_t num_base_steps_;
  std::string cfg_type_;
  std::int32_t fps_;
  std::int32_t motion_rep_dim_;
  std::int32_t global_root_dim_;
  std::int32_t local_root_dim_;
  std::string motion_mask_mode_;
  std::int32_t latent_dim_;
  std::int32_t ff_size_;
  std::int32_t num_layers_;
  std::int32_t num_heads_;
  std::string activation_;
  double dropout_;
  double pe_dropout_;
  bool norm_first_;
  bool use_text_mask_;
  std::int32_t num_text_tokens_;
  std::int32_t num_text_tokens_override_;
  bool input_first_heading_angle_;
  std::vector<std::int32_t> llm_shape_;
  std::int32_t llm_dim_;

  struct sub_module_config {
    std::int32_t input_dim_;
    std::int32_t output_dim_;

    friend void from_json(const nlohmann::json& j, sub_module_config& p) {
      j.at("input_dim").get_to(p.input_dim_);
      j.at("output_dim").get_to(p.output_dim_);
    }
    friend void to_json(nlohmann::json& j, const sub_module_config& p) {
      j["input_dim"]  = p.input_dim_;
      j["output_dim"] = p.output_dim_;
    }
  };

  sub_module_config root_;
  sub_module_config body_;

  void load_from_json(const FSys::path& json_path);

  FSys::path denoiser_root_path_;
  FSys::path denoiser_body_path_;
  FSys::path text_encoder_model_path_;
  FSys::path tokenizer_json_path_;
  FSys::path skeleton_dir_;
  FSys::path stats_path_;

  friend void from_json(const nlohmann::json& j, kimodo_model_config& p) {
    j.at("model_name").get_to(p.model_name_);
    j.at("skeleton_type").get_to(p.skeleton_type_);
    j.at("nb_joints").get_to(p.nb_joints_);
    j.at("num_base_steps").get_to(p.num_base_steps_);
    j.at("cfg_type").get_to(p.cfg_type_);
    j.at("fps").get_to(p.fps_);
    j.at("motion_rep_dim").get_to(p.motion_rep_dim_);
    j.at("global_root_dim").get_to(p.global_root_dim_);
    j.at("local_root_dim").get_to(p.local_root_dim_);
    j.at("motion_mask_mode").get_to(p.motion_mask_mode_);
    j.at("latent_dim").get_to(p.latent_dim_);
    j.at("ff_size").get_to(p.ff_size_);
    j.at("num_layers").get_to(p.num_layers_);
    j.at("num_heads").get_to(p.num_heads_);
    j.at("activation").get_to(p.activation_);
    j.at("dropout").get_to(p.dropout_);
    j.at("pe_dropout").get_to(p.pe_dropout_);
    j.at("norm_first").get_to(p.norm_first_);
    j.at("use_text_mask").get_to(p.use_text_mask_);
    j.at("num_text_tokens").get_to(p.num_text_tokens_);
    j.at("num_text_tokens_override").get_to(p.num_text_tokens_override_);
    j.at("input_first_heading_angle").get_to(p.input_first_heading_angle_);
    j.at("llm_shape").get_to(p.llm_shape_);
    j.at("llm_dim").get_to(p.llm_dim_);
    j.at("root").get_to(p.root_);
    j.at("body").get_to(p.body_);
  }
  friend void to_json(nlohmann::json& j, const kimodo_model_config& p) {
    j["model_name"]                = p.model_name_;
    j["skeleton_type"]             = p.skeleton_type_;
    j["nb_joints"]                 = p.nb_joints_;
    j["num_base_steps"]            = p.num_base_steps_;
    j["cfg_type"]                  = p.cfg_type_;
    j["fps"]                       = p.fps_;
    j["motion_rep_dim"]            = p.motion_rep_dim_;
    j["global_root_dim"]           = p.global_root_dim_;
    j["local_root_dim"]            = p.local_root_dim_;
    j["motion_mask_mode"]          = p.motion_mask_mode_;
    j["latent_dim"]                = p.latent_dim_;
    j["ff_size"]                   = p.ff_size_;
    j["num_layers"]                = p.num_layers_;
    j["num_heads"]                 = p.num_heads_;
    j["activation"]                = p.activation_;
    j["dropout"]                   = p.dropout_;
    j["pe_dropout"]                = p.pe_dropout_;
    j["norm_first"]                = p.norm_first_;
    j["use_text_mask"]             = p.use_text_mask_;
    j["num_text_tokens"]           = p.num_text_tokens_;
    j["num_text_tokens_override"]  = p.num_text_tokens_override_;
    j["input_first_heading_angle"] = p.input_first_heading_angle_;
    j["llm_shape"]                 = p.llm_shape_;
    j["llm_dim"]                   = p.llm_dim_;
    j["root"]                      = p.root_;
    j["body"]                      = p.body_;
  }
};

/// @brief Kimodo 主编排类（对应 Python Kimodo）
///
/// 编排完整推理管线: 文本编码 → 去噪循环 → 运动解码。
/// 子组件均为已实现的 C++ 类。
///
/// 用法:
/// @code
///   auto cfg = std::make_shared<kimodo_model_config>();
///   cfg->load_from_json("path/to/model_config.json");
///   auto kmd = std::make_shared<kimodo>();
///   kmd->load(cfg);
///   auto output = kmd->generate({"a person walks"}, {120}, 50);
/// @endcode
///
/// @note _multiprompt（多段拼接）暂不实现。
class kimodo {
  // ---- 核心组件 ----
  classifier_free_guided_model denoiser_;          ///< CFG 包装的去噪器
  diffusion diffusion_;                            ///< 扩散过程
  ddim_sampler sampler_;                           ///< DDIM 采样器
  std::shared_ptr<kimodo_motion_rep> motion_rep_;  ///< 运动表示
  std::shared_ptr<skeleton_base> skeleton_;        ///< 骨骼 (用于输出转换)

  // ---- 文本编码器 ----
  std::shared_ptr<LLM2Vec> text_encoder_;

  // ---- 配置 ----
  std::string cfg_type_default_{"separated"};
  float fps_{30.0f};
  std::int64_t llm_dim_{4096};  ///< 文本嵌入维度

  // ---- 内部辅助 ----
  struct text_encoding_result {
    MatrixXfRow text_feat;  ///< [B, D] 文本嵌入（已平坦化，实际形状 [B*1, D]）
    MatrixXbRow text_pad_mask;     ///< [B, 1] 文本 mask
  };

  /// @brief 编码文本（对应 Python _generate 中的 self.text_encoder(texts)）
  text_encoding_result encode_texts(const std::vector<std::string>& texts);

  /// @brief 单步去噪（对应 Python denoising_step）
  /// @param map_tensor 时间步映射表（由 generate_internal 预计算，避免每步重复 space_timesteps）
  /// @return [B*T, D] t-1 步的噪声运动（平坦化）
  MatrixXfRow denoising_step(
      const MatrixXfRow& motion, const MatrixXbRow& pad_mask, const MatrixXfRow& text_feat,
      const MatrixXbRow& text_pad_mask, std::int64_t t, const std::vector<int64_t>& map_tensor,
      const std::vector<float>& first_heading_angle, const MatrixXfRow& motion_mask,
      const MatrixXfRow& observed_motion, const std::vector<float>& cfg_weight, const std::string& cfg_type
  );

  /// @brief 完整去噪循环（对应 Python _generate）
  /// @return [B*T, D] 去噪后的运动（平坦化，已标准化）
  MatrixXfRow generate_internal(
      const std::vector<std::string>& texts, std::int64_t max_frames, std::int64_t num_denoising_steps,
      const MatrixXbRow& pad_mask, const std::vector<float>& first_heading_angle, const MatrixXfRow& motion_mask,
      const MatrixXfRow& observed_motion, const std::vector<float>& cfg_weight, const std::string& cfg_type
  );

 public:
  kimodo()                         = default;
  ~kimodo()                        = default;

  // 禁止拷贝
  kimodo(const kimodo&)            = delete;
  kimodo& operator=(const kimodo&) = delete;

  /// @brief 从模型配置加载模型
  ///
  /// @param config 模型配置（共享指针），包含所有路径、维度、标志等设置
  void load(std::shared_ptr<kimodo_model_config> config);

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
  /// @return 已裁剪的运动输出列表（每个 batch 元素一个，仅保留有效帧）
  std::vector<motion_output> generate(
      const std::vector<std::string>& prompts, const std::vector<std::int64_t>& num_frames,
      std::int64_t num_denoising_steps, const std::vector<float>& cfg_weight = {2.0f, 2.0f},
      const std::vector<float>& first_heading_angle = {}, const MatrixXfRow& motion_mask = {},
      const MatrixXfRow& observed_motion = {}, const std::string& cfg_type = ""
  );

  /// @brief 简单版本：单个提示、单样本
  motion_output generate(
      const std::string& prompt, std::int64_t num_frames, std::int64_t num_denoising_steps,
      const std::vector<float>& cfg_weight = {2.0f, 2.0f}, float first_heading_angle = 0.0f
  );

  [[nodiscard]] bool is_valid() const {
    return denoiser_.is_valid() && diffusion_.is_valid() && motion_rep_ != nullptr;
  }

  [[nodiscard]] const std::shared_ptr<kimodo_motion_rep>& motion_rep() const { return motion_rep_; }
  [[nodiscard]] const std::shared_ptr<skeleton_base>& skeleton() const { return skeleton_; }
  [[nodiscard]] float fps() const { return fps_; }
};

}  // namespace doodle::ai
