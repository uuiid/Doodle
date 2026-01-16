#include "sparsemax.h"

#include <ATen/core/Tensor.h>

namespace doodle::ai {

torch::Tensor sparsemax::forward(torch::autograd::AutogradContext* ctx, const torch::Tensor& input) {
  auto l_input_sizes             = input.sizes().vec();
  ctx->saved_data["input_sizes"] = l_input_sizes;

  auto l_reshaped                = input.reshape({-1, l_input_sizes.back()});
  auto l_max_row                 = std::get<0>(l_reshaped.max(-1, /*keepdim=*/true));
  auto l_shifted                 = l_reshaped - l_max_row;

  auto l_sorted                  = std::get<0>(l_shifted.sort(-1, /*descending=*/true));
  auto l_cumsum                  = l_sorted.cumsum(-1);
  auto l_rhos                    = torch::arange(1, l_sorted.size(-1) + 1, l_sorted.options()).unsqueeze(0);
  auto l_support                 = (l_sorted * l_rhos) > (l_cumsum - 1);
  auto l_k                       = l_support.sum(-1).unsqueeze(-1);
  auto l_tau_sum                 = l_cumsum.gather(-1, l_k - 1);
  auto l_tau                     = (l_tau_sum - 1) / l_k.to(l_tau_sum.dtype());

  auto l_output                  = torch::clamp_min(l_shifted - l_tau, 0);
  ctx->save_for_backward({l_support});
  return l_output.reshape(l_input_sizes);
}

torch::autograd::variable_list sparsemax::backward(
    torch::autograd::AutogradContext* ctx, torch::autograd::variable_list&& grad_outputs
) {
  auto l_saved      = ctx->get_saved_variables();
  auto l_support_b  = l_saved[0];
  auto l_grad       = grad_outputs[0];
  auto l_input_size = ctx->saved_data["input_sizes"].toIntVector();

  auto l_grad_2d    = l_grad.reshape({-1, l_grad.size(-1)});
  auto l_support    = l_support_b.to(l_grad_2d.dtype());
  auto l_denom      = l_support.sum(-1, /*keepdim=*/true).clamp_min(1e-20);
  auto l_sum_grad   = (l_grad_2d * l_support).sum(-1, /*keepdim=*/true) / l_denom;
  auto l_grad_input = l_support * (l_grad_2d - l_sum_grad);
  return {l_grad_input.reshape(l_input_size)};
}
}  // namespace doodle::ai