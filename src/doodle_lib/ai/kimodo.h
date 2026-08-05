//
// Created by TD on 25-7-23.
//
#pragma once

#include <doodle_lib/ai/cfg_type.h>
#include <doodle_lib/ai/classifier_free_guided_model.h>
#include <doodle_lib/ai/diffusion.h>
#include <doodle_lib/ai/fwd.h>
#include <doodle_lib/ai/llm2vec.h>
#include <doodle_lib/ai/motion_mask_mode.h>
#include <doodle_lib/ai/motion_rep/constraint_set.h>
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
  cfg_type cfg_type_{cfg_type::separated};
  std::int32_t fps_;
  std::int32_t motion_rep_dim_;
  std::int32_t global_root_dim_;
  std::int32_t local_root_dim_;
  motion_mask_mode motion_mask_mode_{motion_mask_mode::none};
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

/// @brief 单段生成参数（对应 _multiprompt 中每个 segment）
struct generate_segment_args {
  std::string text_{};
  std::int64_t num_frames_{120};
  std::int32_t num_denoising_steps_{50};
  std::vector<float> cfg_weight_{2.0f, 2.0f};
  cfg_type cfg_type_{cfg_type::separated};
  float first_heading_angle_{0.0f};
  std::int32_t num_transition_frames_{10};
  bool post_processing_{false};
  float root_margin_{0.0f};
  nlohmann::json constraint_lst_{};
  std::vector<constraint_set_ptr> constraints_{};

  friend void from_json(const nlohmann::json& j, generate_segment_args& p) {
    if (j.contains("text") && j.at("text").is_string())
      j.at("text").get_to(p.text_);
    if (j.contains("num_frames") && j.at("num_frames").is_number_integer())
      j.at("num_frames").get_to(p.num_frames_);
    if (j.contains("num_denoising_steps") && j.at("num_denoising_steps").is_number_integer())
      j.at("num_denoising_steps").get_to(p.num_denoising_steps_);
    if (j.contains("cfg_weight") && j.at("cfg_weight").is_array() && j.at("cfg_weight").size() > 0)
      j.at("cfg_weight").get_to(p.cfg_weight_);
    if (j.contains("cfg_type") && j.at("cfg_type").is_string())
      j.at("cfg_type").get_to(p.cfg_type_);
    if (j.contains("first_heading_angle") && j.at("first_heading_angle").is_number())
      j.at("first_heading_angle").get_to(p.first_heading_angle_);
    if (j.contains("num_transition_frames") && j.at("num_transition_frames").is_number_integer())
      j.at("num_transition_frames").get_to(p.num_transition_frames_);
    if (j.contains("post_processing") && j.at("post_processing").is_boolean())
      j.at("post_processing").get_to(p.post_processing_);
    if (j.contains("root_margin") && j.at("root_margin").is_number())
      j.at("root_margin").get_to(p.root_margin_);
    if (j.contains("constraint_lst") && j.at("constraint_lst").is_array())
      j.at("constraint_lst").get_to(p.constraint_lst_);
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
///   auto output = kmd->generate({{"a person walks", 120, 50}});
/// @endcode
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
  float fps_{30.0f};
  std::int64_t llm_dim_{4096};  ///< 文本嵌入维度

  // ---- 内部辅助 ----
  struct text_encoding_result {
    MatrixXfRow text_feat;      ///< [B, D] 文本嵌入（已平坦化，实际形状 [B*1, D]）
    MatrixXbRow text_pad_mask;  ///< [B, 1] 文本 mask
  };

  /// @brief 过渡准备结果
  struct transition_prep_result {
    MatrixXfRow prev_smooth_root_2d;           ///< [1, 2] 新段起点的平滑根位置 (x, z)
    float heading_val;                         ///< 新段朝向角（弧度）
    std::vector<constraint_set_ptr> trans_constraints;  ///< 过渡约束（FullBody + EndEffector），供后处理使用
  };

  /// @brief 编码文本（对应 Python _generate 中的 self.text_encoder(texts)）
  text_encoding_result encode_texts(const std::vector<std::string>& texts);

  /// @brief 单步去噪（对应 Python denoising_step）
  /// @param map_tensor 时间步映射表（由 generate_internal 预计算，避免每步重复 space_timesteps）
  /// @return [B*T, D] t-1 步的噪声运动（平坦化）
  MatrixXfRow denoising_step(
      const MatrixXfRow& motion, const MatrixXbRow& pad_mask, const MatrixXfRow& text_feat,
      const MatrixXbRow& text_pad_mask, std::int64_t t, const std::vector<int64_t>& map_tensor,
      const std::vector<float>& first_heading_angle, const MatrixXfRow& motion_mask, const MatrixXfRow& observed_motion,
      const std::vector<float>& cfg_weight, cfg_type cfg_type_val
  );

  /// @brief 完整去噪循环（对应 Python _generate）
  /// @return [B*T, D] 去噪后的运动（平坦化，已标准化）
  MatrixXfRow generate_internal(
      const std::vector<std::string>& texts, std::int64_t max_frames, std::int64_t num_denoising_steps,
      const MatrixXbRow& pad_mask, const std::vector<float>& first_heading_angle, const MatrixXfRow& motion_mask,
      const MatrixXfRow& observed_motion, const std::vector<float>& cfg_weight, cfg_type cfg_type_val
  );

  /// @brief 准备段间过渡：截断上一段、构建过渡约束、拼接观测运动
  /// @param[in,out] generated_motions 已生成段列表（最后一段被截断）
  /// @param[out] prev_latest_frames 上一段末尾过渡帧 [1*nb_transition, D]
  /// @param[in] prev_nb_transition 上一段过渡帧数
  /// @param[in,out] observed_motion 段观测运动（被拼接过渡部分）
  /// @param[in,out] motion_mask_f 运动掩码（被拼接过渡部分）
  /// @param[in,out] num_frame 帧数（增加过渡帧）
  /// @param[in] nb_transition 当前段过渡帧数
  transition_prep_result prepare_transition(
      std::vector<MatrixXfRow>& generated_motions, MatrixXfRow& prev_latest_frames,
      std::int64_t prev_nb_transition, MatrixXfRow& observed_motion, MatrixXfRow& motion_mask_f,
      std::int64_t& num_frame, std::int64_t nb_transition
  );

  /// @brief 混合过渡帧：平移回原位、拆分、Alpha 混合
  /// @param[in,out] motion 生成的运动 [1*(nb_transition+num_frame), D] → 截断为 [1*num_frame, D]
  /// @param[in] prev_latest_frames 上一段末尾过渡帧 [1*nb_transition, D]
  /// @param[in] prev_smooth_root_2d 平移量 [1, 2]
  /// @param[in] nb_transition 过渡帧数
  /// @param[in] num_frame 段帧数（含过渡帧）
  /// @return 混合后的过渡帧 [1*nb_transition, D]
  MatrixXfRow blend_transition(
      MatrixXfRow& motion, const MatrixXfRow& prev_latest_frames, const MatrixXfRow& prev_smooth_root_2d,
      std::int64_t nb_transition, std::int64_t num_frame
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

  /// @brief 多段顺序生成运动（对应 Python _multiprompt）
  ///
  /// 按顺序生成多个运动段，段间自动创建平滑过渡。
  ///
  /// @param segments 段参数列表（至少 1 个元素）
  /// @return 拼接后的完整运动输出
  motion_output generate(const std::vector<generate_segment_args>& segments);

  [[nodiscard]] bool is_valid() const {
    return denoiser_.is_valid() && diffusion_.is_valid() && motion_rep_ != nullptr;
  }

  [[nodiscard]] const std::shared_ptr<kimodo_motion_rep>& motion_rep() const { return motion_rep_; }
  [[nodiscard]] const std::shared_ptr<skeleton_base>& skeleton() const { return skeleton_; }
  [[nodiscard]] float fps() const { return fps_; }
};

}  // namespace doodle::ai
