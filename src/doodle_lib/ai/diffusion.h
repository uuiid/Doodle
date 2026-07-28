//
// Created by TD on 25-7-23.
//
#pragma once

#include <doodle_lib/core/global_function.h>

#include <Eigen/Dense>
#include <cstdint>
#include <utility>
#include <vector>

namespace doodle::ai {

struct kimodo_model_config;

/// @brief Cosine-schedule 扩散过程：betas、alphas 和 DDIM 步映射（对应 Python Diffusion）
///
/// 管理完整的 cosine beta schedule 以及针对子采样去噪步数的所有预计算 diffusion 变量。
/// calc_diffusion_vars() 会根据当前去噪步数重新计算所有内部 buffer。
class diffusion {
  // ---- 基础配置 ----
  std::int64_t num_base_steps_{};
  std::int64_t current_num_steps_{};

  // ---- 基础 (完整) cosine schedule ----
  std::vector<float> betas_base_;
  std::vector<float> alphas_cumprod_base_;

  // ---- 子采样 schedule (由 calc_diffusion_vars 更新) ----
  std::vector<float> betas_;
  std::vector<float> alphas_;
  std::vector<float> alphas_cumprod_;
  std::vector<float> alphas_cumprod_prev_;
  std::vector<float> sqrt_recip_alphas_cumprod_;
  std::vector<float> sqrt_recipm1_alphas_cumprod_;
  std::vector<float> posterior_variance_;
  std::vector<float> sqrt_alphas_cumprod_;
  std::vector<float> sqrt_one_minus_alphas_cumprod_;

 public:
  diffusion() = default;

  /// @brief 构造并初始化 cosine beta schedule
  /// @param config 模型配置（共享指针），提供 num_base_steps 等参数
  explicit diffusion(std::shared_ptr<kimodo_model_config> config) { init(std::move(config)); }

  /// @brief (重新)初始化扩散调度
  /// @param config 模型配置（共享指针），提供 num_base_steps 等参数
  void init(std::shared_ptr<kimodo_model_config> config);

  // ======================================================================
  // 核心函数
  // ======================================================================

  /// @brief 对去噪步数做子采样，返回 (use_timesteps, map_tensor)
  ///
  /// use_timesteps: [num_denoising_steps] 用于 calc_diffusion_vars 的子采样时间步索引
  /// map_tensor:    [num_denoising_steps] 将步索引 i 映射到基础时间步索引
  ///
  /// 对应 Python: space_timesteps()
  std::pair<std::vector<std::int64_t>, std::vector<std::int64_t>> space_timesteps(
      std::int64_t num_denoising_steps
  ) const;

  /// @brief 根据子采样时间步更新所有 diffusion 变量（对应 Python calc_diffusion_vars）
  ///
  /// 计算: betas, alphas, alphas_cumprod, alphas_cumprod_prev,
  ///       sqrt_recip_alphas_cumprod, sqrt_recipm1_alphas_cumprod,
  ///       posterior_variance, sqrt_alphas_cumprod, sqrt_one_minus_alphas_cumprod
  void calc_diffusion_vars(const std::vector<std::int64_t>& use_timesteps);

  /// @brief 前向加噪: x_t = sqrt(alpha_cumprod[t]) * x_start + sqrt(1 - alpha_cumprod[t]) * noise
  /// @param x_start [N, D] 干净运动
  /// @param t 时间步索引
  /// @param noise [N, D] 噪声（可选，为空时自动生成标准正态噪声）
  /// @return [N, D] 加噪后的运动
  Eigen::MatrixXf q_sample(const Eigen::MatrixXf& x_start, std::int64_t t, const Eigen::MatrixXf& noise = {}) const;

  // ======================================================================
  // 访问器（供 ddim_sampler 使用）
  // ======================================================================

  [[nodiscard]] float sqrt_recip_alphas_cumprod(std::int64_t t) const {
    return sqrt_recip_alphas_cumprod_.at(static_cast<std::size_t>(t));
  }
  [[nodiscard]] float sqrt_recipm1_alphas_cumprod(std::int64_t t) const {
    return sqrt_recipm1_alphas_cumprod_.at(static_cast<std::size_t>(t));
  }
  [[nodiscard]] float alphas_cumprod_prev(std::int64_t t) const {
    return alphas_cumprod_prev_.at(static_cast<std::size_t>(t));
  }

  [[nodiscard]] std::int64_t num_base_steps() const { return num_base_steps_; }
  [[nodiscard]] std::int64_t current_num_steps() const { return current_num_steps_; }
  [[nodiscard]] bool is_valid() const { return num_base_steps_ > 0; }
};

/// @brief 确定性 DDIM 采样器（eta = 0）（对应 Python DDIMSampler）
///
/// 执行单步 DDIM 反向去噪:
///   eps = (sqrt_recip_ac[t] * x_t - pred_xstart) / sqrt_recipm1_ac[t]
///   x_{t-1} = pred_xstart * sqrt(alpha_bar_prev[t]) + sqrt(1 - alpha_bar_prev[t]) * eps
///
/// 注意: 调用前需确保 diffusion_.calc_diffusion_vars() 已被调用。
class ddim_sampler {
  diffusion* diffusion_{};

 public:
  ddim_sampler() = default;
  explicit ddim_sampler(diffusion& diff) : diffusion_(&diff) {}

  /// @brief 设置关联的 diffusion 对象
  void set_diffusion(diffusion& diff) { diffusion_ = &diff; }

  /// @brief 获取关联的 diffusion 对象
  [[nodiscard]] diffusion& get_diffusion() { return *diffusion_; }
  [[nodiscard]] const diffusion& get_diffusion() const { return *diffusion_; }

  /// @brief 执行单步 DDIM 采样（对应 Python __call__）
  ///
  /// 假设 calc_diffusion_vars 已在外部被调用，本函数不再重复调用。
  ///
  /// @param x_t [B*T, D] 当前 t 步的噪声运动（平坦化）
  /// @param pred_xstart [B*T, D] 模型预测的干净运动
  /// @param t 当前步索引（所有 batch 元素相同）
  /// @return [B*T, D] t-1 步的运动
  Eigen::MatrixXf step(const Eigen::MatrixXf& x_t, const Eigen::MatrixXf& pred_xstart, std::int64_t t) const;

  [[nodiscard]] bool is_valid() const { return diffusion_ != nullptr && diffusion_->is_valid(); }
};

}  // namespace doodle::ai
