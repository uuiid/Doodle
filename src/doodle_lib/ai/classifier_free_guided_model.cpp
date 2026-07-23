//
// Created by TD on 25-7-20.
//
#include "classifier_free_guided_model.h"

#include <doodle_core/exception/exception.h>

#include <fmt/format.h>

namespace doodle::ai {

void classifier_free_guided_model::load(
    const FSys::path& root_model_dir,
    const FSys::path& body_model_dir,
    std::int64_t latent_dim,
    std::int64_t num_text_tokens,
    bool use_text_mask,
    const std::shared_ptr<motion_rep_base>& motion_rep,
    const std::string& motion_mask_mode,
    bool input_first_heading_angle
) {
  model_.load(
      root_model_dir, body_model_dir,
      latent_dim, num_text_tokens, use_text_mask,
      motion_rep,
      motion_mask_mode, input_first_heading_angle
  );
}

namespace {
/// @brief 将 vector 沿自身拼接 n 次
template <typename T>
std::vector<T> repeat_vector(const std::vector<T>& vec, std::size_t n) {
  if (vec.empty()) return {};
  std::vector<T> result;
  result.reserve(vec.size() * n);
  for (std::size_t i = 0; i < n; ++i) {
    result.insert(result.end(), vec.begin(), vec.end());
  }
  return result;
}
}  // namespace

Eigen::MatrixXf classifier_free_guided_model::forward(
    const std::vector<float>& cfg_weight,
    const Eigen::MatrixXf& x,
    const MatrixXb& x_pad_mask,
    const Eigen::MatrixXf& text_feat,
    const MatrixXb& text_feat_pad_mask,
    const std::vector<std::int64_t>& timesteps,
    const std::vector<float>& first_heading_angle,
    const Eigen::MatrixXf& motion_mask,
    const Eigen::MatrixXf& observed_motion,
    const std::string& cfg_type
) {
  const auto& actual_cfg_type = cfg_type.empty() ? cfg_type_default_ : cfg_type;
  DOODLE_CHICK(is_valid(), "classifier_free_guided_model 未初始化，请先调用 load()");

  const Eigen::Index batch_size     = x_pad_mask.rows();
  const Eigen::Index time_steps     = x_pad_mask.cols();
  const Eigen::Index total_frames   = batch_size * time_steps;
  const Eigen::Index text_token_cnt = text_feat_pad_mask.cols();
  const Eigen::Index llm_dim        = text_feat.cols();

  // ======================================================================
  // nocfg: 无引导，直接转发
  // ======================================================================
  if (actual_cfg_type == "nocfg") {
    return model_.forward(
        x, x_pad_mask, text_feat, text_feat_pad_mask, timesteps,
        first_heading_angle, motion_mask, observed_motion
    );
  }

  // ======================================================================
  // regular: out_uncond + w * (out_cond - out_uncond)
  // ======================================================================
  if (actual_cfg_type == "regular") {
    DOODLE_CHICK(cfg_weight.size() == 1, "regular CFG 需要单个 cfg_weight，当前有 {} 个", cfg_weight.size());
    const float w = cfg_weight[0];
    
    // --- x: [2*B*T, input_dim] ---
    Eigen::MatrixXf x_cat(total_frames * 2, x.cols());
    x_cat.topRows(total_frames)    = x;
    x_cat.bottomRows(total_frames) = x;

    // --- x_pad_mask: [2*B, T] ---
    MatrixXb x_pad_mask_cat(batch_size * 2, time_steps);
    x_pad_mask_cat.topRows(batch_size)    = x_pad_mask;
    x_pad_mask_cat.bottomRows(batch_size) = x_pad_mask;

    // --- text_feat: 条件 = 原文，无条件 = 零 ---
    const Eigen::Index text_total = text_feat.rows();
    Eigen::MatrixXf text_feat_cat(text_total * 2, llm_dim);
    text_feat_cat.topRows(text_total)    = text_feat;
    text_feat_cat.bottomRows(text_total).setZero();

    // --- text_feat_pad_mask: 条件 = 原文，无条件 = false ---
    MatrixXb text_pad_cat(batch_size * 2, text_token_cnt);
    text_pad_cat.topRows(batch_size)    = text_feat_pad_mask;
    text_pad_cat.bottomRows(batch_size).setConstant(false);

    // --- timesteps: [2*B] ---
    const auto timesteps_cat = repeat_vector(timesteps, 2);

    // --- first_heading_angle: [2*B] 或空 ---
    const auto heading_cat = repeat_vector(first_heading_angle, 2);

    // --- motion_mask: 条件 = 原文，无条件 = 零 ---
    // --- observed_motion: 两份都是原文 ---
    Eigen::MatrixXf motion_mask_cat;
    Eigen::MatrixXf observed_motion_cat;
    const bool has_motion_mask = motion_mask.size() > 0;
    if (has_motion_mask) {
      motion_mask_cat.resize(total_frames * 2, motion_mask.cols());
      motion_mask_cat.topRows(total_frames)    = motion_mask;
      motion_mask_cat.bottomRows(total_frames).setZero();

      observed_motion_cat.resize(total_frames * 2, observed_motion.cols());
      observed_motion_cat.topRows(total_frames)    = observed_motion;
      observed_motion_cat.bottomRows(total_frames) = observed_motion;
    }

    // --- 一次 batch forward ---
    const auto out_cond_uncond = model_.forward(
        x_cat, x_pad_mask_cat, text_feat_cat, text_pad_cat, timesteps_cat,
        heading_cat,
        has_motion_mask ? motion_mask_cat : Eigen::MatrixXf{},
        has_motion_mask ? observed_motion_cat : Eigen::MatrixXf{}
    );

    DOODLE_CHICK(
        out_cond_uncond.rows() == total_frames * 2,
        "regular CFG forward 输出行数 {} 不匹配期望 {}",
        out_cond_uncond.rows(), total_frames * 2
    );

    // --- 拆分条件/无条件输出并应用 CFG ---
    const auto out       = out_cond_uncond.topRows(total_frames);
    const auto out_uncond = out_cond_uncond.bottomRows(total_frames);

    return out_uncond.array() + w * (out.array() - out_uncond.array());
  }

  // ======================================================================
  // separated: out_uncond + w_text*(out_text - out_uncond) + w_constraint*(out_constraint - out_uncond)
  // ======================================================================
  if (actual_cfg_type == "separated") {
    DOODLE_CHICK(
        cfg_weight.size() == 2,
        "separated CFG 需要两个 cfg_weight (text, constraint)，当前有 {} 个",
        cfg_weight.size()
    );
    const float w_text       = cfg_weight[0];
    const float w_constraint = cfg_weight[1];

    // --- x: [3*B*T, input_dim] ---
    Eigen::MatrixXf x_cat(total_frames * 3, x.cols());
    x_cat.topRows(total_frames)                   = x;
    x_cat.middleRows(total_frames, total_frames)  = x;
    x_cat.bottomRows(total_frames)                = x;

    // --- x_pad_mask: [3*B, T] ---
    MatrixXb x_pad_mask_cat(batch_size * 3, time_steps);
    x_pad_mask_cat.topRows(batch_size)                   = x_pad_mask;
    x_pad_mask_cat.middleRows(batch_size, batch_size)    = x_pad_mask;
    x_pad_mask_cat.bottomRows(batch_size)                = x_pad_mask;

    // --- text_feat: copy0=原文(text), copy1=零(constraint), copy2=零(uncond) ---
    const Eigen::Index text_total = text_feat.rows();
    Eigen::MatrixXf text_feat_cat(text_total * 3, llm_dim);
    text_feat_cat.topRows(text_total)                   = text_feat;
    text_feat_cat.middleRows(text_total, text_total).setZero();
    text_feat_cat.bottomRows(text_total).setZero();

    // --- text_feat_pad_mask: copy0=原文, copy1=false, copy2=false ---
    MatrixXb text_pad_cat(batch_size * 3, text_token_cnt);
    text_pad_cat.topRows(batch_size)                   = text_feat_pad_mask;
    text_pad_cat.middleRows(batch_size, batch_size).setConstant(false);
    text_pad_cat.bottomRows(batch_size).setConstant(false);

    // --- timesteps: [3*B] ---
    const auto timesteps_cat = repeat_vector(timesteps, 3);

    // --- first_heading_angle: [3*B] 或空 ---
    const auto heading_cat = repeat_vector(first_heading_angle, 3);

    // --- motion_mask: copy0=零, copy1=原文(constraint), copy2=零 ---
    // --- observed_motion: 三份都是原文 ---
    Eigen::MatrixXf motion_mask_cat;
    Eigen::MatrixXf observed_motion_cat;
    const bool has_motion_mask = motion_mask.size() > 0;
    if (has_motion_mask) {
      motion_mask_cat.resize(total_frames * 3, motion_mask.cols());
      motion_mask_cat.topRows(total_frames).setZero();
      motion_mask_cat.middleRows(total_frames, total_frames) = motion_mask;
      motion_mask_cat.bottomRows(total_frames).setZero();

      observed_motion_cat.resize(total_frames * 3, observed_motion.cols());
      observed_motion_cat.topRows(total_frames)                   = observed_motion;
      observed_motion_cat.middleRows(total_frames, total_frames)  = observed_motion;
      observed_motion_cat.bottomRows(total_frames)                = observed_motion;
    }

    // --- 一次 batch forward ---
    const auto out_cond_uncond = model_.forward(
        x_cat, x_pad_mask_cat, text_feat_cat, text_pad_cat, timesteps_cat,
        heading_cat,
        has_motion_mask ? motion_mask_cat : Eigen::MatrixXf{},
        has_motion_mask ? observed_motion_cat : Eigen::MatrixXf{}
    );

    DOODLE_CHICK(
        out_cond_uncond.rows() == total_frames * 3,
        "separated CFG forward 输出行数 {} 不匹配期望 {}",
        out_cond_uncond.rows(), total_frames * 3
    );

    // --- 拆分为 text / constraint / uncond ---
    const auto out_text       = out_cond_uncond.topRows(total_frames);
    const auto out_constraint = out_cond_uncond.middleRows(total_frames, total_frames);
    const auto out_uncond     = out_cond_uncond.bottomRows(total_frames);

    return out_uncond.array() +
           w_text * (out_text.array() - out_uncond.array()) +
           w_constraint * (out_constraint.array() - out_uncond.array());
  }

  // ======================================================================
  // 未知 cfg_type
  // ======================================================================
  DOODLE_CHICK(false, "未知的 cfg_type: {}", actual_cfg_type);
}

}  // namespace doodle::ai
