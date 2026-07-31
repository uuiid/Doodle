//
// Created by TD on 25-7-23.
//
#include "kimodo.h"

#include <doodle_core/exception/exception.h>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <random>
#include <spdlog/spdlog.h>

namespace doodle::ai {

void kimodo_model_config::load_from_json(const FSys::path& json_path) {
  DOODLE_CHICK(FSys::exists(json_path), "kimodo_model_config json 文件不存在: {}", json_path.string());
  auto l_json_path = json_path;
  if (FSys::is_directory(json_path)) l_json_path /= "model_config.json";
  DOODLE_CHICK(FSys::exists(l_json_path), "kimodo_model_config json 文件不存在: {}", l_json_path.string());
  *this                    = nlohmann::json::parse(FSys::ifstream{l_json_path}).get<kimodo_model_config>();
  denoiser_root_path_      = l_json_path.parent_path() / "root";
  denoiser_body_path_      = l_json_path.parent_path() / "body";
  text_encoder_model_path_ = l_json_path.parent_path() / "text_encoder" / "model.onnx";
  tokenizer_json_path_     = l_json_path.parent_path() / "text_encoder" / "tokenizer.json";
  skeleton_dir_            = l_json_path.parent_path() / "skeleton";
  stats_path_              = l_json_path.parent_path() / "stats";
}

// ======================================================================
// load
// ======================================================================
void kimodo::load(std::shared_ptr<kimodo_model_config> config) {
  DOODLE_CHICK(config != nullptr, "kimodo_model_config 为空");
  SPDLOG_INFO("kimodo::load 开始...");

  llm_dim_  = config->llm_dim_;
  fps_      = static_cast<float>(config->fps_);

  // ---- Step 1: 初始化骨骼 ----
  skeleton_ = skeleton_base::create_soma_skeleton_30(config->skeleton_dir_);
  DOODLE_CHICK(skeleton_ != nullptr && skeleton_->is_valid(), "SOMASkeleton30 加载失败");

  // ---- Step 2: 初始化运动表示 ----
  motion_rep_ = std::make_shared<kimodo_motion_rep>(skeleton_, config);
  DOODLE_CHICK(motion_rep_ != nullptr && motion_rep_->motion_rep_dim() > 0, "kimodo_motion_rep 初始化失败");

  SPDLOG_INFO(
      "kimodo: motion_rep 初始化完成: dim={}, global_root_dim={}, body_dim={}", motion_rep_->motion_rep_dim(),
      motion_rep_->global_root_dim(), motion_rep_->body_dim()
  );

  // ---- Step 3: 初始化扩散过程 ----
  diffusion_.init(config);
  sampler_.set_diffusion(diffusion_);
  SPDLOG_INFO("kimodo: diffusion 初始化完成: num_base_steps={}", config->num_base_steps_);

  // ---- Step 4: 初始化去噪器（含 CFG 包装） ----
  denoiser_.load(config, motion_rep_);
  SPDLOG_INFO("kimodo: denoiser (CFG) 加载完成");

  // ---- Step 5: 初始化文本编码器 ----
  text_encoder_ = std::make_shared<LLM2Vec>(config->text_encoder_model_path_, config->tokenizer_json_path_);
  DOODLE_CHICK(text_encoder_ != nullptr, "LLM2Vec 初始化失败");
  SPDLOG_INFO("kimodo: text_encoder 初始化完成");

  text_encoder_->load_onnx();
  denoiser_.load_onnx();

  SPDLOG_INFO("kimodo::load 完成");
}

// ======================================================================
// encode_texts: 文本编码（对应 Python _generate 中的 text_encoder 调用）
// ======================================================================
kimodo::text_encoding_result kimodo::encode_texts(const std::vector<std::string>& texts) {
  const Eigen::Index B = static_cast<Eigen::Index>(texts.size());

  // ---- 对每个文本调用 LLM2Vec 获取 pooled embedding ----
  // Python LLM2VecEncoder 返回 [B, 1, D] 的单个 token 嵌入
  // C++ LLM2Vec 对每个文本返回 std::vector<float> [D]
  MatrixXfRow text_feat(B, llm_dim_);
  text_feat.setZero();

  for (Eigen::Index b = 0; b < B; ++b) {
    const auto& txt = texts[static_cast<std::size_t>(b)];

    if (txt.empty()) {
      // 空文本 → 全零（对应 Python: text_feat[empty_text_mask] = 0）
      text_feat.row(b).setZero();
    } else {
      // LLM2Vec 使用空 instruction（对应 Python: text_encoder("", text)）
      auto emb = (*text_encoder_)("", txt);
      if (emb.empty()) {
        SPDLOG_WARN("LLM2Vec 对文本 '{}' 返回空嵌入，使用零向量", txt);
        text_feat.row(b).setZero();
      } else {
        DOODLE_CHICK(
            static_cast<Eigen::Index>(emb.size()) == llm_dim_, "LLM2Vec 输出维度 {} 不匹配预期 llm_dim {}", emb.size(),
            llm_dim_
        );
        for (Eigen::Index d = 0; d < llm_dim_; ++d) {
          text_feat(b, d) = emb[static_cast<std::size_t>(d)];
        }
      }
    }
  }

  // ---- 创建 text_pad_mask: [B, 1] 全 true（Python: lengths = [1, 1, ...]） ----
  MatrixXbRow text_pad_mask(B, 1);
  text_pad_mask.setConstant(true);

  SPDLOG_INFO("kimodo: 编码 {} 个文本, text_feat shape [{}x{}]", B, text_feat.rows(), text_feat.cols());

  return {std::move(text_feat), std::move(text_pad_mask)};
}

// ======================================================================
// denoising_step: 单步去噪（对应 Python denoising_step）
// ======================================================================
MatrixXfRow kimodo::denoising_step(
    const MatrixXfRow& motion, const MatrixXbRow& pad_mask, const MatrixXfRow& text_feat,
    const MatrixXbRow& text_pad_mask, std::int64_t t, const std::vector<int64_t>& map_tensor,
    const std::vector<float>& first_heading_angle, const MatrixXfRow& motion_mask, const MatrixXfRow& observed_motion,
    const std::vector<float>& cfg_weight, cfg_type cfg_type_val
) {
  // ---- 获取映射后的时间步（space_timesteps + calc_diffusion_vars 已在 generate_internal 中预计算） ----
  const std::int64_t t_map      = map_tensor[static_cast<std::size_t>(t)];

  // ---- 创建 timesteps 向量（所有 batch 元素使用相同的 t_map） ----
  const Eigen::Index batch_size = pad_mask.rows();
  std::vector<std::int64_t> timesteps_vec(static_cast<std::size_t>(batch_size), t_map);

  // ---- CFG 去噪器推理 ----
  // Python: pred_clean = self.denoiser(cfg_weight, motion, pad_mask, text_feat,
  //                                      text_pad_mask, t_map, first_heading_angle,
  //                                      motion_mask, observed_motion, cfg_type=cfg_type)
  MatrixXfRow pred_clean = denoiser_.forward(
      cfg_weight, motion, pad_mask, text_feat, text_pad_mask, timesteps_vec, first_heading_angle, motion_mask,
      observed_motion, cfg_type_val
  );

  // ---- DDIM 采样 ----
  // Python: x_tm1 = self.sampler(use_timesteps, motion, pred_clean, t)
  MatrixXfRow x_tm1 = sampler_.step(motion, pred_clean, t);

  return x_tm1;
}

// ======================================================================
// generate_internal: 完整去噪循环（对应 Python _generate）
// ======================================================================
MatrixXfRow kimodo::generate_internal(
    const std::vector<std::string>& texts, std::int64_t max_frames, std::int64_t num_denoising_steps,
    const MatrixXbRow& pad_mask, const std::vector<float>& first_heading_angle, const MatrixXfRow& motion_mask,
    const MatrixXfRow& observed_motion, const std::vector<float>& cfg_weight, cfg_type cfg_type_val
) {
  const Eigen::Index batch_size = pad_mask.rows();
  const std::int64_t D          = motion_rep_->motion_rep_dim();
  const Eigen::Index total      = batch_size * max_frames;

  DOODLE_CHICK(
      static_cast<Eigen::Index>(texts.size()) == batch_size, "文本数 {} 不匹配 batch_size {}", texts.size(), batch_size
  );

  // ---- Step 1: 文本编码 ----
  auto [text_feat, text_pad_mask] = encode_texts(texts);
  // text_feat: [B, D] → 作为 [B*1, D] 使用（max_text_len=1）
  // text_pad_mask: [B, 1]

  // ---- Step 2: motion_mask 类型转换（bool → float） ----
  // Python: if motion_mask is not None:
  //            if motion_mask.dtype == torch.bool: motion_mask = 1 * motion_mask
  MatrixXfRow motion_mask_f;
  if (motion_mask.size() > 0) {
    motion_mask_f = motion_mask;
  }

  // ---- Step 3: 初始化噪声 ----
  // Python: cur_mot = torch.randn([B, T, D])
  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<float> dist(0.0f, 1.0f);

  MatrixXfRow cur_mot(total, D);
  for (Eigen::Index i = 0; i < cur_mot.size(); ++i) {
    cur_mot(i) = dist(gen);
  }

  // ---- Step 4: 预计算扩散时间步（仅一次，避免每步重复 space_timesteps + calc_diffusion_vars） ----
  auto [use_timesteps, map_tensor] = diffusion_.space_timesteps(num_denoising_steps);
  diffusion_.calc_diffusion_vars(use_timesteps);

  // ---- Step 5: 去噪循环 ----
  // Python: for i in reversed(range(num_denoising_steps)):
  SPDLOG_INFO("kimodo: 开始去噪循环, num_denoising_steps={}", num_denoising_steps);

  for (std::int64_t i = num_denoising_steps - 1; i >= 0; --i) {
    auto start_time = std::chrono::high_resolution_clock::now();
    cur_mot         = denoising_step(
        cur_mot, pad_mask, text_feat, text_pad_mask, i, map_tensor, first_heading_angle, motion_mask_f, observed_motion,
        cfg_weight, cfg_type_val
    );
    SPDLOG_INFO("kimodo: 去噪步 {} 完成 time {:%S}", i, std::chrono::high_resolution_clock::now() - start_time);
  }

  SPDLOG_INFO("kimodo: 去噪循环完成");
  return cur_mot;
}

// ======================================================================
// generate: 主入口（对应 Python __call__，不含 multi_prompt）
// ======================================================================
std::vector<motion_output> kimodo::generate(
    const std::vector<std::string>& prompts, const std::vector<std::int64_t>& num_frames,
    std::int64_t num_denoising_steps, const std::vector<float>& cfg_weight,
    const std::vector<float>& first_heading_angle, const std::vector<constraint_set_ptr>& constraints,
    cfg_type cfg_type_val
) {
  DOODLE_CHICK(is_valid(), "kimodo 未加载或加载失败");
  DOODLE_CHICK(!prompts.empty(), "prompts 不能为空");
  DOODLE_CHICK(
      prompts.size() == num_frames.size(), "prompts 数量 {} 与 num_frames 数量 {} 不匹配", prompts.size(),
      num_frames.size()
  );

  const Eigen::Index B          = static_cast<Eigen::Index>(prompts.size());
  const std::int64_t max_frames = *std::max_element(num_frames.begin(), num_frames.end());
  const std::int64_t D          = motion_rep_->motion_rep_dim();

  // ---- Step 1: 创建 motion_pad_mask（无约束时为全有效，Python length_to_mask） ----
  Eigen::VectorXi lengths_vec(B);
  for (Eigen::Index b = 0; b < B; ++b) {
    lengths_vec(b) = static_cast<int>(num_frames[static_cast<std::size_t>(b)]);
  }
  MatrixXbRow pad_mask = length_to_mask(lengths_vec, max_frames);

  // ---- Step 2: 处理 first_heading_angle ----
  // Python:
  //   if first_heading_angle is None:  start at 0 angle
  //   else:  repeat if scalar -> [B]
  std::vector<float> heading(B, 0.0f);
  if (first_heading_angle.size() == 1) {
    // 标量 → 广播到所有 batch
    std::fill(heading.begin(), heading.end(), first_heading_angle[0]);
  } else if (static_cast<Eigen::Index>(first_heading_angle.size()) == B) {
    heading = first_heading_angle;
  } else if (!first_heading_angle.empty()) {
    DOODLE_CHICK(false, "first_heading_angle 大小 {} 不匹配 batch_size {}", first_heading_angle.size(), B);
  }
  // else: 全零（已初始化）

  // ---- Step 3: 处理约束条件（constraint_set_ptr → motion_mask + observed_motion） ----
  MatrixXfRow motion_mask_f, observed_motion_f;
  if (!constraints.empty()) {
    kimodo_motion_rep::constraint_dicts dicts;
    for (const auto& c : constraints) {
      c->update_constraints(dicts.data_dict, dicts.index_dict);
    }
    std::vector<kimodo_motion_rep::constraint_dicts> constraint_dicts_per_sample = {std::move(dicts)};

    auto [obs, mask] = motion_rep_->create_conditions_from_constraints_batched(
        constraint_dicts_per_sample, lengths_vec, true /*to_normalize*/
    );
    observed_motion_f = std::move(obs);
    motion_mask_f     = mask.cast<float>();
  }

  // ---- Step 4: 去噪循环 ----
  const std::int64_t total_frames = B * max_frames;

  MatrixXfRow motion              = generate_internal(
      prompts, max_frames, num_denoising_steps, pad_mask, heading, motion_mask_f, observed_motion_f, cfg_weight,
      cfg_type_val
  );

  // ---- Step 5: 反标准化 ----
  // Python: motion = self.motion_rep.unnormalize(motion)
  DOODLE_CHICK(
      motion.rows() == total_frames && motion.cols() == D, "生成的运动 shape [{}x{}] 不匹配预期 [{}x{}]", motion.rows(),
      motion.cols(), total_frames, D
  );

  motion               = motion_rep_->unnormalize(motion);

  // ---- Step 6: 解码为运动输出 ----
  // Python: output = self.motion_rep.inverse(motion, is_normalized=True, return_numpy=False)
  // 注意：motion 已反标准化，所以 is_normalized=False
  motion_output output = motion_rep_->decode(motion, false /*is_normalized*/, B, max_frames);

  // ---- Step 7: 按 num_frames 裁剪有效帧（仅保留各 batch 实际长度） ----
  // decode 返回 [B*T, ...] 平坦化结果，T=max_frames
  // 对每个 batch 元素 b，仅保留前 num_frames[b] 帧，丢弃 padding 噪声
  std::vector<motion_output> trimmed_outputs;
  trimmed_outputs.reserve(static_cast<std::size_t>(B));

  for (Eigen::Index b = 0; b < B; ++b) {
    const Eigen::Index start_row = b * max_frames;
    const Eigen::Index count     = static_cast<Eigen::Index>(num_frames[static_cast<std::size_t>(b)]);

    motion_output out;
    out.local_rot_mats      = output.local_rot_mats.middleRows(start_row, count).eval();
    out.global_rot_mats     = output.global_rot_mats.middleRows(start_row, count).eval();
    out.posed_joints        = output.posed_joints.middleRows(start_row, count).eval();
    out.root_positions      = output.root_positions.middleRows(start_row, count).eval();
    out.smooth_root_pos     = output.smooth_root_pos.middleRows(start_row, count).eval();
    out.foot_contacts       = output.foot_contacts.middleRows(start_row, count).eval();
    out.global_root_heading = output.global_root_heading.middleRows(start_row, count).eval();
    trimmed_outputs.push_back(std::move(out));
  }

  // ---- Step 8: SOMA 骨骼输出转换 ----
  // Python: if isinstance(self.skeleton, SOMASkeleton30):
  //             output = self.skeleton.output_to_SOMASkeleton77(output)
  // TODO: output_to_SOMASkeleton77 转换暂未实现
  // 对应的 Python 方法将 SOMA30 关节重新映射到 SOMA77 关节
  // 若需要此转换，需在 skeleton_base 中添加对应方法

  SPDLOG_INFO("kimodo::generate 完成: batch_size={}, max_frames={}, motion_rep_dim={}", B, max_frames, D);

  return trimmed_outputs;
}

// ======================================================================
// generate: 简单单样本版本
// ======================================================================
motion_output kimodo::generate(
    const std::string& prompt, std::int64_t num_frames, std::int64_t num_denoising_steps,
    const std::vector<float>& cfg_weight, float first_heading_angle
) {
  auto results = generate(
      std::vector<std::string>{prompt}, std::vector<std::int64_t>{num_frames}, num_denoising_steps, cfg_weight,
      {first_heading_angle}
  );
  DOODLE_CHICK(results.size() == 1, "单样本 generate 应返回恰好 1 个结果，实际: {}", results.size());
  return std::move(results[0]);
}

}  // namespace doodle::ai
