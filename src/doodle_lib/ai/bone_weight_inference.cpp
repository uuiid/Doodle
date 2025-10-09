#include "bone_weight_inference.h"

#include <mimalloc.h>

#pragma comment(linker, "/include:mi_version")

#include <torch/csrc/api/include/torch/torch.h>
namespace doodle::ai {

struct GraphSample {
  // node features: [num_nodes, in_dim]
  torch::Tensor x;
  // normalized adjacency: [num_nodes, num_nodes] (float)
  // we will use dense adjacency for simplicity
  torch::Tensor adj;
  // label distribution per node: [num_nodes, num_bones]
  torch::Tensor y;
  // optional: node mask or other metadata
};

torch::Tensor normalize_adjacency(const torch::Tensor& A) {
  // A: [N, N], float
  auto I            = torch::eye(A.size(0), A.options());
  auto A_hat        = A + I;
  auto deg          = A_hat.sum(1);  // [N]
  // avoid div by zero
  auto deg_inv_sqrt = torch::pow(deg + 1e-12, -0.5);
  auto D_inv_sqrt   = torch::diag(deg_inv_sqrt);
  auto norm         = torch::matmul(torch::matmul(D_inv_sqrt, A_hat), D_inv_sqrt);
  return norm;
}

void test() { mi_version(); }

}  // namespace doodle::ai