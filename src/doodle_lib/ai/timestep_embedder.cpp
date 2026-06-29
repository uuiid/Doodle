//
// Created by TD on 25-6-29.
//
#include "timestep_embedder.h"

#include <doodle_core/exception/exception.h>

namespace doodle::ai {

void TimestepEmbedder::init(
    std::int64_t latent_dim,
    const PositionalEncoding* pos_encoder,
    const FSys::path& linear1_weight,
    const FSys::path& linear1_bias,
    const FSys::path& linear2_weight,
    const FSys::path& linear2_bias
) {
  latent_dim_            = latent_dim;
  sequence_pos_encoder_  = pos_encoder;
  DOODLE_CHICK(sequence_pos_encoder_ != nullptr, "TimestepEmbedder: pos_encoder 为空");
  DOODLE_CHICK(sequence_pos_encoder_->d_model() == latent_dim, "TimestepEmbedder: PE 维度 {} 不匹配 latent_dim {}", sequence_pos_encoder_->d_model(), latent_dim);

  linear1_.load(linear1_weight, linear1_bias);
  linear2_.load(linear2_weight, linear2_bias);

  DOODLE_CHICK(linear1_.in_features() == latent_dim && linear1_.out_features() == latent_dim,
      "TimestepEmbedder linear1 维度不匹配: 期望 [{},{}], 实际 [{},{}]",
      latent_dim, latent_dim, linear1_.in_features(), linear1_.out_features());
  DOODLE_CHICK(linear2_.in_features() == latent_dim && linear2_.out_features() == latent_dim,
      "TimestepEmbedder linear2 维度不匹配: 期望 [{},{}], 实际 [{},{}]",
      latent_dim, latent_dim, linear2_.in_features(), linear2_.out_features());
}

Eigen::MatrixXf TimestepEmbedder::forward(const std::vector<std::int64_t>& timesteps) const {
  DOODLE_CHICK(is_valid(), "TimestepEmbedder 未初始化");

  auto batch_size = static_cast<Eigen::Index>(timesteps.size());

  // 从位置编码表中查询时间步编码: [B, latent_dim]
  // 对应 Python: sequence_pos_encoder.pe.transpose(0, 1)[timesteps]
  // Python 中 pe shape: [1, max_len, D], transpose(0,1) -> [max_len, 1, D], [timesteps] -> [B, 1, D]
  // 我们在 C++ 中用 lookup 得到 [B, D]，然后 unsqueeze 为 [B, 1, D]
  Eigen::MatrixXf pe_out = sequence_pos_encoder_->lookup(timesteps);  // [B, latent_dim]

  // 通过 MLP: Linear -> SiLU -> Linear
  Eigen::MatrixXf h = linear1_.forward(pe_out);  // [B, latent_dim]
  // SiLU 激活: x * sigmoid(x)
  h = h.array() * (1.0f / (1.0f + (-h.array()).exp()));
  h = linear2_.forward(h);  // [B, latent_dim]

  // 添加序列维度: [B, 1, latent_dim]
  // 返回平坦化 [B*1, latent_dim] = [B, latent_dim]
  return h;
}

}  // namespace doodle::ai
