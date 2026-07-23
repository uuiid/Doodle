//
// Created by TD on 25-7-23.
//
#include "diffusion.h"

#include <doodle_core/exception/exception.h>

#include <cmath>
#include <cstdlib>
#include <numbers>
#include <random>
#include <spdlog/spdlog.h>

namespace doodle::ai {

// ======================================================================
// 辅助函数: cosine beta schedule
// ======================================================================

namespace {

/// @brief alpha_bar(t) = cos((t + 0.008) / 1.008 * pi / 2)^2
[[nodiscard]] double alpha_bar(double t) {
  const double arg = (t + 0.008) / 1.008 * std::numbers::pi_v<double> / 2.0;
  return std::cos(arg) * std::cos(arg);
}

/// @brief 生成 cosine beta schedule
/// @param num_diffusion_timesteps 总步数
/// @param max_beta beta 上限
/// @return [num_diffusion_timesteps] betas
[[nodiscard]] std::vector<float> get_beta_schedule(
    std::int64_t num_diffusion_timesteps,
    float max_beta = 0.999f
) {
  std::vector<float> betas(static_cast<std::size_t>(num_diffusion_timesteps));
  for (std::int64_t i = 0; i < num_diffusion_timesteps; ++i) {
    const double t1 = static_cast<double>(i) / static_cast<double>(num_diffusion_timesteps);
    const double t2 = static_cast<double>(i + 1) / static_cast<double>(num_diffusion_timesteps);

    const double ratio = alpha_bar(t2) / alpha_bar(t1);
    double beta       = 1.0 - ratio;
    if (beta > static_cast<double>(max_beta)) beta = static_cast<double>(max_beta);
    betas[static_cast<std::size_t>(i)] = static_cast<float>(beta);
  }
  return betas;
}

/// @brief 累积乘积: result[i] = prod(data[0..i])
[[nodiscard]] std::vector<float> cumprod(const std::vector<float>& data) {
  std::vector<float> result(data.size());
  double acc = 1.0;
  for (std::size_t i = 0; i < data.size(); ++i) {
    acc *= static_cast<double>(data[i]);
    result[i] = static_cast<float>(acc);
  }
  return result;
}

/// @brief 安全的 1/sqrt(x)，x 被 clamp 到 >= min_val
[[nodiscard]] float rsqrt_clamped(float x, float min_val = 1e-9f) {
  if (x < min_val) x = min_val;
  return 1.0f / std::sqrt(x);
}

}  // namespace

// ======================================================================
// diffusion 实现
// ======================================================================

void diffusion::init(std::int64_t num_base_steps) {
  DOODLE_CHICK(num_base_steps > 0, "num_base_steps 必须大于 0，实际为 {}", num_base_steps);
  num_base_steps_ = num_base_steps;

  // ---- 基础 cosine beta schedule ----
  betas_base_ = get_beta_schedule(num_base_steps);

  // ---- 基础累积 alpha: alpha_cumprod_base[i] = prod(1 - beta_base[j] for j in 0..i) ----
  // 先把 (1 - beta) 算出来
  std::vector<float> alphas_base(betas_base_.size());
  for (std::size_t i = 0; i < betas_base_.size(); ++i) {
    alphas_base[i] = 1.0f - betas_base_[i];
  }
  alphas_cumprod_base_ = cumprod(alphas_base);

  // ---- 用完整步数初始化子采样 buffer ----
  std::vector<std::int64_t> full_timesteps(num_base_steps);
  for (std::int64_t i = 0; i < num_base_steps; ++i) full_timesteps[static_cast<std::size_t>(i)] = i;
  calc_diffusion_vars(full_timesteps);

  SPDLOG_INFO("Diffusion 初始化完成: num_base_steps={}", num_base_steps);
}

std::pair<std::vector<std::int64_t>, std::vector<std::int64_t>> diffusion::space_timesteps(
    std::int64_t num_denoising_steps
) const {
  DOODLE_CHICK(num_base_steps_ > 0, "Diffusion 未初始化");

  const std::int64_t nsteps_train = num_base_steps_;
  // Python: frac_stride = (nsteps_train - 1) / max(1, num_denoising_steps - 1)
  const double frac_stride =
      static_cast<double>(nsteps_train - 1) / std::max(std::int64_t{1}, num_denoising_steps - 1);

  std::vector<std::int64_t> use_timesteps(static_cast<std::size_t>(num_denoising_steps));
  for (std::int64_t i = 0; i < num_denoising_steps; ++i) {
    // Python: torch.round(torch.arange(...) * frac_stride)
    const double val = static_cast<double>(i) * frac_stride;
    use_timesteps[static_cast<std::size_t>(i)] =
        std::min(static_cast<std::int64_t>(std::round(val)), nsteps_train - 1);
  }

  // Python: map_tensor = torch.arange(nsteps_train)[use_timesteps]
  // 即 map_tensor[i] = use_timesteps[i] 的前 num_denoising_steps 个元素
  // 此处只生成有效的 num_denoising_steps 个元素
  std::vector<std::int64_t> map_tensor = use_timesteps;

  return {std::move(use_timesteps), std::move(map_tensor)};
}

void diffusion::calc_diffusion_vars(const std::vector<std::int64_t>& use_timesteps) {
  DOODLE_CHICK(!alphas_cumprod_base_.empty(), "Diffusion 未初始化，alphas_cumprod_base_ 为空");

  const auto n = use_timesteps.size();
  current_num_steps_ = static_cast<std::int64_t>(n);

  // 临时函数: 按 use_timesteps 索引取值
  auto index_base = [&](const std::vector<float>& base, std::size_t idx) -> float {
    return base[static_cast<std::size_t>(use_timesteps[idx])];
  };

  // ---- alphas_cumprod = alphas_cumprod_base[use_timesteps] ----
  std::vector<float> alphas_cumprod(n);
  for (std::size_t i = 0; i < n; ++i) {
    alphas_cumprod[i] = index_base(alphas_cumprod_base_, i);
  }

  // ---- last_alpha_cumprod = cat([1.0], alphas_cumprod[:-1]) ----
  // 用于: betas = 1 - alphas_cumprod / last_alpha_cumprod
  alphas_cumprod_prev_.resize(n);
  betas_.resize(n);
  for (std::size_t i = 0; i < n; ++i) {
    const float last_ac = (i == 0) ? 1.0f : alphas_cumprod[i - 1];
    alphas_cumprod_prev_[i] = last_ac;
    // betas = 1 - alphas_cumprod / last_alpha_cumprod
    betas_[i] = (last_ac > 0.0f) ? (1.0f - alphas_cumprod[i] / last_ac) : 0.0f;
  }

  // ---- alphas = 1 - betas ----
  alphas_.resize(n);
  for (std::size_t i = 0; i < n; ++i) {
    alphas_[i] = 1.0f - betas_[i];
  }

  // ---- alphas_cumprod = cumprod(alphas), clamped to >= 1e-9 ----
  // 注意：alphas_cumprod 是重新计算的，不是从 base 索引来的
  {
    double acc = 1.0;
    for (std::size_t i = 0; i < n; ++i) {
      acc *= static_cast<double>(alphas_[i]);
      if (acc < 1e-9) acc = 1e-9;
      // 同时也需要对 alphas_cumprod[i] 做 1e-9 的下界保护
      alphas_cumprod[i] = static_cast<float>(acc);
    }
  }

  // ---- alphas_cumprod_prev = cat([1.0], alphas_cumprod[:-1]) ----
  // 重新计算（因为 alphas_cumprod 变了）
  {
    std::vector<float> new_ac_prev(n);
    for (std::size_t i = 0; i < n; ++i) {
      new_ac_prev[i] = (i == 0) ? 1.0f : alphas_cumprod[i - 1];
    }
    alphas_cumprod_prev_ = std::move(new_ac_prev);
  }

  // ---- sqrt_recip_alphas_cumprod = rsqrt(alphas_cumprod) ----
  //     = 1 / sqrt(alphas_cumprod)
  sqrt_recip_alphas_cumprod_.resize(n);
  for (std::size_t i = 0; i < n; ++i) {
    sqrt_recip_alphas_cumprod_[i] = rsqrt_clamped(alphas_cumprod[i]);
  }

  // ---- sqrt_recipm1_alphas_cumprod = rsqrt(alphas_cumprod / (1 - alphas_cumprod)) ----
  //     = sqrt(1 - alphas_cumprod) / sqrt(alphas_cumprod)
  sqrt_recipm1_alphas_cumprod_.resize(n);
  for (std::size_t i = 0; i < n; ++i) {
    const float denom = alphas_cumprod[i] / (1.0f - alphas_cumprod[i]);
    sqrt_recipm1_alphas_cumprod_[i] = rsqrt_clamped(denom);
  }

  // ---- posterior_variance = betas * (1 - alphas_cumprod_prev) / (1 - alphas_cumprod) ----
  posterior_variance_.resize(n);
  for (std::size_t i = 0; i < n; ++i) {
    const float one_minus_ac     = 1.0f - alphas_cumprod[i];
    const float one_minus_ac_prev = 1.0f - alphas_cumprod_prev_[i];
    posterior_variance_[i] = (one_minus_ac > 0.0f)
                                 ? betas_[i] * one_minus_ac_prev / one_minus_ac
                                 : 0.0f;
  }

  // ---- sqrt_alphas_cumprod = rsqrt(1 / alphas_cumprod) = sqrt(alphas_cumprod) ----
  sqrt_alphas_cumprod_.resize(n);
  for (std::size_t i = 0; i < n; ++i) {
    sqrt_alphas_cumprod_[i] = std::sqrt(alphas_cumprod[i]);
  }

  // ---- sqrt_one_minus_alphas_cumprod = rsqrt(1 / (1 - alphas_cumprod))
  //     = sqrt(1 - alphas_cumprod) ----
  sqrt_one_minus_alphas_cumprod_.resize(n);
  for (std::size_t i = 0; i < n; ++i) {
    const float one_minus_ac = 1.0f - alphas_cumprod[i];
    sqrt_one_minus_alphas_cumprod_[i] = std::sqrt(std::max(one_minus_ac, 0.0f));
  }
}

Eigen::MatrixXf diffusion::q_sample(
    const Eigen::MatrixXf& x_start,
    std::int64_t t,
    const Eigen::MatrixXf& noise
) const {
  DOODLE_CHICK(is_valid(), "Diffusion 未初始化");
  DOODLE_CHICK(static_cast<std::size_t>(t) < sqrt_alphas_cumprod_.size(), "t={} 超出范围", t);

  const auto rows = x_start.rows();
  const auto cols = x_start.cols();

  // 如果没有提供噪声，生成标准正态噪声
  Eigen::MatrixXf noise_used;
  if (noise.size() == 0) {
    static std::mt19937 gen{std::random_device{}()};
    static std::normal_distribution<float> dist{0.0f, 1.0f};
    noise_used = Eigen::MatrixXf::NullaryExpr(rows, cols, [&]() { return dist(gen); });
  } else {
    DOODLE_CHICK(noise.rows() == rows && noise.cols() == cols, "noise shape 不匹配");
    noise_used = noise;
  }

  // x_t = sqrt_alpha_cumprod[t] * x_start + sqrt_one_minus_alphas_cumprod[t] * noise
  const float sqrt_ac   = sqrt_alphas_cumprod_[static_cast<std::size_t>(t)];
  const float sqrt_1mac = sqrt_one_minus_alphas_cumprod_[static_cast<std::size_t>(t)];

  return sqrt_ac * x_start + sqrt_1mac * noise_used;
}

// ======================================================================
// ddim_sampler 实现
// ======================================================================

Eigen::MatrixXf ddim_sampler::step(
    const Eigen::MatrixXf& x_t,
    const Eigen::MatrixXf& pred_xstart,
    std::int64_t t
) const {
  DOODLE_CHICK(diffusion_ != nullptr, "ddim_sampler 未关联 diffusion");
  DOODLE_CHICK(diffusion_->is_valid(), "关联的 diffusion 未初始化");

  const auto rows = x_t.rows();
  const auto cols = x_t.cols();
  DOODLE_CHICK(
      pred_xstart.rows() == rows && pred_xstart.cols() == cols,
      "pred_xstart shape [{}x{}] 不匹配 x_t [{}x{}]",
      pred_xstart.rows(), pred_xstart.cols(), rows, cols
  );

  // ---- 计算 eps ----
  // Python:
  //   eps = (sqrt_recip_alphas_cumprod[t] * x_t - pred_xstart)
  //         / sqrt_recipm1_alphas_cumprod[t]
  const float recip_ac = diffusion_->sqrt_recip_alphas_cumprod(t);
  const float recipm1_ac = diffusion_->sqrt_recipm1_alphas_cumprod(t);

  // eps = (recip_ac * x_t - pred_xstart) / recipm1_ac
  // 注意: 当 recipm1_ac == 0 时（t=0），eps 无定义，此时 x_{t-1} = pred_xstart
  Eigen::MatrixXf eps;
  if (std::abs(recipm1_ac) > 1e-9f) {
    eps = (recip_ac * x_t - pred_xstart) / recipm1_ac;
  } else {
    // t=0 时直接返回 pred_xstart
    return pred_xstart;
  }

  // ---- 计算 x_{t-1} ----
  // Python:
  //   alpha_bar_prev = alphas_cumprod_prev[t]
  //   x = pred_xstart * sqrt(alpha_bar_prev) + sqrt(1 - alpha_bar_prev) * eps
  const float ac_prev = diffusion_->alphas_cumprod_prev(t);
  const float sqrt_ac_prev     = std::sqrt(ac_prev);
  const float sqrt_one_minus_ac = std::sqrt(std::max(1.0f - ac_prev, 0.0f));

  return sqrt_ac_prev * pred_xstart + sqrt_one_minus_ac * eps;
}

}  // namespace doodle::ai
