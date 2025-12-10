#include "cross_attention_bone_weight.h"

#include <doodle_lib/ai/load_fbx.h>

#include <ATen/core/TensorBody.h>
#include <memory>
#include <vector>

namespace doodle::ai {

struct LocalPointTransformerBlockImpl : torch::nn::Module {
  torch::nn::Linear fc_q, fc_k, fc_v, out;
  torch::nn::LayerNorm norm;
  explicit LocalPointTransformerBlockImpl(std::int64_t in_dim, std::int64_t out_dim)
      : fc_q(nullptr),
        fc_k(nullptr),
        fc_v(nullptr),
        out(nullptr),
        norm(nullptr)

  {
    // 初始化层
    fc_q = register_module("fc_q", torch::nn::Linear{in_dim, out_dim});
    fc_k = register_module("fc_k", torch::nn::Linear{in_dim, out_dim});
    fc_v = register_module("fc_v", torch::nn::Linear{in_dim, out_dim});
    out  = register_module("out", torch::nn::Linear{in_dim, out_dim});
    norm = register_module("norm", torch::nn::LayerNorm{std::vector<std::int64_t>{out_dim}});
  }

  torch::Tensor forward(const torch::Tensor& x, const torch::Tensor& neigh_idx) {
    auto l_size = x.sizes();  // [B, N, C]
    auto l_K    = neigh_idx.size(2);
    auto q      = fc_q->forward(x);
    auto k      = fc_k->forward(x);
    auto v      = fc_v->forward(x);
    // 注意力计算
    {
      std::vector<torch::Tensor> k_rows{};  // 定义k_nei
      std::vector<torch::Tensor> v_rows{};  // 定义v_nei
      k_rows.reserve(l_size[0]);
      v_rows.reserve(l_size[0]);
      for (std::int64_t b = 0; b < l_size[0]; ++b) {
        k_rows.push_back(k[b][neigh_idx[b]].unsqueeze(0));  // 高级索引
        v_rows.push_back(v[b][neigh_idx[b]].unsqueeze(0));
      }
    }

    // 实现前向传播逻辑
    return x;  // 示例返回输入张量
  }
};

TORCH_MODULE(LocalPointTransformerBlock);

std::shared_ptr<cross_attention_bone_weight> cross_attention_bone_weight::train(
    const std::vector<FSys::path>& in_fbx_files, const FSys::path& in_output_path
) {
  auto l_ret = std::make_shared<cross_attention_bone_weight>();

  return l_ret;
}

}  // namespace doodle::ai