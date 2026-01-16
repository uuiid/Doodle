#include "sparsemax.h"

#include <ATen/core/Tensor.h>

namespace doodle::ai {

torch::Tensor sparsemax::forward(torch::autograd::AutogradContext* ctx, const torch::Tensor& input, int dim) {
  TORCH_CHECK(input.defined(), "sparsemax: input must be defined");
  TORCH_CHECK(input.dim() >= 1, "sparsemax: input must have at least 1 dimension");

  ctx->saved_data["input_sizes"] = input.sizes().vec();
  int64_t ndim                   = input.dim();
  int64_t dim64                  = static_cast<int64_t>(dim);
  if (dim64 < 0) {
    dim64 += ndim;
  }
  TORCH_CHECK(dim64 >= 0 && dim64 < ndim, "sparsemax: dim out of range");
  ctx->saved_data["dim"] = dim64;

  torch::Tensor permuted = input;
  std::vector<int64_t> perm;
  std::vector<int64_t> inv_perm;
  if (dim64 != (ndim - 1)) {
    perm.reserve(ndim);
    for (int64_t i = 0; i < ndim; ++i) {
      if (i != dim64) {
        perm.push_back(i);
      }
    }
    perm.push_back(dim64);

    inv_perm.resize(ndim);
    for (int64_t i = 0; i < ndim; ++i) {
      inv_perm[perm[i]] = i;
    }
    permuted = input.permute(perm);
  }

  auto reshaped  = permuted.reshape({-1, permuted.size(-1)});
  auto max_row   = std::get<0>(reshaped.max(-1, /*keepdim=*/true));
  auto shifted   = reshaped - max_row;

  auto sorted    = std::get<0>(shifted.sort(-1, /*descending=*/true));
  auto cumsum    = sorted.cumsum(-1);
  auto rhos      = torch::arange(1, sorted.size(-1) + 1, sorted.options()).unsqueeze(0);
  auto support   = (sorted * rhos) > (cumsum - 1);
  auto k         = support.sum(-1).unsqueeze(-1);
  auto tau_sum   = cumsum.gather(-1, k - 1);
  auto tau       = (tau_sum - 1) / k.to(tau_sum.dtype());

  auto output_2d = torch::clamp_min(shifted - tau, 0);
  ctx->save_for_backward({support});

  auto output_perm = output_2d.reshape(permuted.sizes());
  if (dim64 != (ndim - 1)) {
    return output_perm.permute(inv_perm);
  }
  return output_perm;
}

torch::autograd::variable_list sparsemax::backward(
    torch::autograd::AutogradContext* ctx, torch::autograd::variable_list grad_outputs
) {
  auto saved       = ctx->get_saved_variables();
  auto support_b   = saved[0];
  auto grad        = grad_outputs[0];
  auto input_sizes = ctx->saved_data["input_sizes"].toIntVector();
  auto dim64       = ctx->saved_data["dim"].toInt();

  int64_t ndim     = static_cast<int64_t>(input_sizes.size());
  TORCH_CHECK(ndim >= 1, "sparsemax: invalid saved input_sizes");
  TORCH_CHECK(dim64 >= 0 && dim64 < ndim, "sparsemax: invalid saved dim");

  torch::Tensor grad_perm = grad;
  std::vector<int64_t> perm;
  std::vector<int64_t> inv_perm;
  if (dim64 != (ndim - 1)) {
    perm.reserve(ndim);
    for (int64_t i = 0; i < ndim; ++i) {
      if (i != dim64) {
        perm.push_back(i);
      }
    }
    perm.push_back(dim64);

    inv_perm.resize(ndim);
    for (int64_t i = 0; i < ndim; ++i) {
      inv_perm[perm[i]] = i;
    }
    grad_perm = grad.permute(perm);
  }

  auto d          = support_b.size(-1);
  auto grad_2d    = grad_perm.reshape({-1, d});
  auto support    = support_b.to(grad_2d.dtype());
  auto denom      = support.sum(-1, /*keepdim=*/true).clamp_min(1e-20);
  auto sum_grad   = (grad_2d * support).sum(-1, /*keepdim=*/true) / denom;
  auto grad_in_2d = support * (grad_2d - sum_grad);

  std::vector<int64_t> permuted_sizes;
  if (dim64 != (ndim - 1)) {
    permuted_sizes.reserve(ndim);
    for (auto idx : perm) {
      permuted_sizes.push_back(input_sizes[idx]);
    }
  }
  auto grad_in_perm = grad_in_2d.reshape((dim64 != (ndim - 1)) ? permuted_sizes : input_sizes);
  if (dim64 != (ndim - 1)) {
    return {grad_in_perm.permute(inv_perm), torch::Tensor()};
  }
  return {grad_in_perm, torch::Tensor()};
}
}  // namespace doodle::ai