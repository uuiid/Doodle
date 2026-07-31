//
// Created by TD on 25-7-17.
//
#include "twostage_denoiser.h"

#include <doodle_core/exception/exception.h>

#include "kimodo.h"
#include <fmt/format.h>
#include <spdlog/spdlog.h>


namespace doodle::ai {

void twostage_denoiser::load_onnx() {
  DOODLE_CHICK(motion_rep_.lock() != nullptr, "twostage_denoiser motion_rep_ 为空");
  DOODLE_CHICK(
      global_root_dim_ > 0 && local_root_dim_ > 0 && input_dim_ > 0,
      "twostage_denoiser motion_rep 维度无效: input_dim={}, global_root_dim={}, local_root_dim={}", input_dim_,
      global_root_dim_, local_root_dim_
  );

  root_model_.load_onnx();
  body_model_.load_onnx();
}

void twostage_denoiser::load(
    std::shared_ptr<kimodo_model_config> config, const std::shared_ptr<motion_rep_base>& motion_rep
) {
  DOODLE_CHICK(config != nullptr, "kimodo_model_config 为空");
  DOODLE_CHICK(motion_rep != nullptr, "motion_rep 不能为空");
  DOODLE_CHICK(
      motion_rep->motion_rep_dim() > 0 && motion_rep->global_root_dim() > 0,
      "motion_rep 维度无效: motion_rep_dim={}, global_root_dim={}", motion_rep->motion_rep_dim(),
      motion_rep->global_root_dim()
  );

  motion_rep_                        = motion_rep;
  input_dim_                         = motion_rep->motion_rep_dim();
  global_root_dim_                   = motion_rep->global_root_dim();
  local_root_dim_                    = motion_rep->local_root_dim();
  motion_mask_mode_                  = config->motion_mask_mode_;

  const bool will_concatenate        = (motion_mask_mode_ == motion_mask_mode::concat);

  // ---- 计算 root 模型维度 ----
  const std::int64_t root_input_dim  = will_concatenate ? input_dim_ * 2 : input_dim_;
  const std::int64_t root_output_dim = global_root_dim_;

  SPDLOG_INFO(
      "twostage_denoiser 加载 root_model: input_dim={}, output_dim={}, latent_dim={}", root_input_dim, root_output_dim,
      config->latent_dim_
  );

  // ---- 加载根节点模型 ----
  root_model_.load(config->denoiser_root_path_, config);

  // ---- 计算 local_motion_rep_dim: 用局部根节点替换全局根节点后的维度 ----
  // local_motion_rep_dim = input_dim - global_root_dim + local_root_dim
  const std::int64_t local_motion_rep_dim = input_dim_ - global_root_dim_ + local_root_dim_;

  // ---- 计算 body 模型维度 ----
  // body stage 总是使用局部根节点 + 身体（concat 模式时额外拼接 motion_mask/observed_motion）
  const std::int64_t body_input_dim       = local_motion_rep_dim + (will_concatenate ? input_dim_ : 0);
  const std::int64_t body_output_dim      = input_dim_ - global_root_dim_;

  SPDLOG_INFO(
      "twostage_denoiser 加载 body_model: input_dim={}, output_dim={}, latent_dim={}, "
      "local_motion_rep_dim={}",
      body_input_dim, body_output_dim, config->latent_dim_, local_motion_rep_dim
  );

  // ---- 加载身体模型 ----
  body_model_.load(config->denoiser_body_path_, config);
  SPDLOG_INFO(
      "twostage_denoiser 加载完成: input_dim={}, global_root_dim={}, local_root_dim={}, "
      "motion_mask_mode={}",
      input_dim_, global_root_dim_, local_root_dim_, motion_mask_mode_
  );
}

MatrixXfRow twostage_denoiser::forward(
    const MatrixXfRow& x, const MatrixXbRow& x_pad_mask, const MatrixXfRow& text_feat,
    const MatrixXbRow& text_feat_pad_mask, const std::vector<std::int64_t>& timesteps,
    const std::vector<float>& first_heading_angle, const MatrixXfRow& motion_mask,
    const MatrixXfRow& observed_motion
) {
  DOODLE_CHICK(is_valid(), "twostage_denoiser 未初始化");

  const auto batch_size   = static_cast<Eigen::Index>(x_pad_mask.rows());
  const auto time_steps   = static_cast<Eigen::Index>(x_pad_mask.cols());
  const auto total_frames = batch_size * time_steps;

  DOODLE_CHICK(
      x.rows() == total_frames && x.cols() == input_dim_, "x shape [{}x{}] 不匹配期望 [{}x{}]", x.rows(), x.cols(),
      total_frames, input_dim_
  );

  // ---- 处理 motion_mask (concat 模式) ----
  // Python:
  //   if motion_mask_mode == "concat":
  //     if motion_mask is None or observed_motion is None:
  //       motion_mask = zeros_like(x)
  //       observed_motion = zeros_like(x)
  //     x = x * (1 - mask) + observed_motion * mask
  //     x_extended = cat([x, mask], axis=-1)
  //   else:
  //     x_extended = x
  MatrixXfRow x_used;
  MatrixXfRow x_extended;
  MatrixXfRow motion_mask_used;

  if (motion_mask_mode_ == motion_mask_mode::concat) {
    // 确定 motion_mask 和 observed_motion
    if (motion_mask.size() == 0 || observed_motion.size() == 0) {
      motion_mask_used = MatrixXfRow::Zero(total_frames, input_dim_);
      const auto obs   = MatrixXfRow::Zero(total_frames, input_dim_);
      x_used           = x;  // x = x * 1 + 0 = x
      // x_extended = cat([x, zero_mask], axis=-1)
      x_extended.resize(total_frames, input_dim_ * 2);
      x_extended.leftCols(input_dim_)  = x;
      x_extended.rightCols(input_dim_) = motion_mask_used;
    } else {
      DOODLE_CHICK(
          motion_mask.rows() == total_frames && motion_mask.cols() == input_dim_,
          "motion_mask shape [{}x{}] 不匹配期望 [{}x{}]", motion_mask.rows(), motion_mask.cols(), total_frames,
          input_dim_
      );
      DOODLE_CHICK(
          observed_motion.rows() == total_frames && observed_motion.cols() == input_dim_,
          "observed_motion shape [{}x{}] 不匹配期望 [{}x{}]", observed_motion.rows(), observed_motion.cols(),
          total_frames, input_dim_
      );
      motion_mask_used = motion_mask;
      // x = x * (1 - mask) + observed_motion * mask
      x_used = x.array() * (1.0f - motion_mask_used.array()) + observed_motion.array() * motion_mask_used.array();
      // x_extended = cat([x, mask], axis=-1)
      x_extended.resize(total_frames, input_dim_ * 2);
      x_extended.leftCols(input_dim_)  = x_used;
      x_extended.rightCols(input_dim_) = motion_mask_used;
    }
  } else {
    x_used     = x;
    x_extended = x;
  }

  // ---- Stage 1: 预测全局根节点运动 ----
  // root_motion_pred [B*T, global_root_dim]
  const MatrixXfRow root_motion_pred =
      root_model_.forward(x_extended, x_pad_mask, text_feat, text_feat_pad_mask, timesteps, first_heading_angle);

  DOODLE_CHICK(
      root_motion_pred.rows() == total_frames && root_motion_pred.cols() == global_root_dim_,
      "root_motion_pred shape [{}x{}] 不匹配期望 [{}x{}]", root_motion_pred.rows(), root_motion_pred.cols(),
      total_frames, global_root_dim_
  );

  // ---- 计算 lengths (每个序列的有效帧数) ----
  Eigen::VectorXi lengths(batch_size);
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    lengths(b) = static_cast<int>(x_pad_mask.row(b).count());
  }

  // ---- 全局根节点 → 局部根节点转换 ----
  auto motion_rep = motion_rep_.lock();
  DOODLE_CHICK(motion_rep != nullptr, "motion_rep 已失效（原始 shared_ptr 已释放）");

  MatrixXfRow root_motion_local = motion_rep->global_root_to_local_root(
      root_motion_pred, true, batch_size, time_steps, lengths
  );  // [B*T, local_root_dim]

  DOODLE_CHICK(
      root_motion_local.rows() == total_frames && root_motion_local.cols() == local_root_dim_,
      "root_motion_local shape [{}x{}] 不匹配期望 [{}x{}]", root_motion_local.rows(), root_motion_local.cols(),
      total_frames, local_root_dim_
  );

  // ---- 提取身体运动: body_x = x[..., body_slice] ----
  // body_slice = [global_root_dim : input_dim]
  const auto body_dim          = input_dim_ - global_root_dim_;
  const MatrixXfRow body_x = x_used.rightCols(body_dim);  // [B*T, body_dim]

  // ---- 拼接局部根节点 + 身体: x_new = cat([root_motion_local, body_x], axis=-1) ----
  // x_new: [B*T, local_root_dim + body_dim] = [B*T, local_motion_rep_dim]
  MatrixXfRow x_new(total_frames, local_root_dim_ + body_dim);
  x_new.leftCols(local_root_dim_) = root_motion_local;
  x_new.rightCols(body_dim)       = body_x;

  // ---- 处理 body stage 的 mask (concat 模式) ----
  MatrixXfRow x_new_extended;
  if (motion_mask_mode_ == motion_mask_mode::concat) {
    x_new_extended.resize(total_frames, x_new.cols() + input_dim_);
    x_new_extended.leftCols(x_new.cols()) = x_new;
    x_new_extended.rightCols(input_dim_)  = motion_mask_used;
  } else {
    x_new_extended = x_new;
  }

  // ---- Stage 2: 预测身体运动 ----
  // predicted_body [B*T, body_output_dim] = [B*T, input_dim - global_root_dim]
  const MatrixXfRow predicted_body =
      body_model_.forward(x_new_extended, x_pad_mask, text_feat, text_feat_pad_mask, timesteps, first_heading_angle);

  DOODLE_CHICK(
      predicted_body.rows() == total_frames && predicted_body.cols() == body_dim,
      "predicted_body shape [{}x{}] 不匹配期望 [{}x{}]", predicted_body.rows(), predicted_body.cols(), total_frames,
      body_dim
  );

  // ---- 拼接输出: cat([root_motion_pred, predicted_body], axis=-1) ----
  // output: [B*T, input_dim]
  MatrixXfRow output(total_frames, input_dim_);
  output.leftCols(global_root_dim_) = root_motion_pred;
  output.rightCols(body_dim)        = predicted_body;

  return output;
}

}  // namespace doodle::ai
