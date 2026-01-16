#include "sparsemax.h"

#include <ATen/core/Tensor.h>

namespace doodle::ai {

torch::Tensor sparsemax::forward(torch::autograd::AutogradContext* ctx, const torch::Tensor& input) {
  auto l_input_size = input.sizes();
  auto l_reshaped   = input.view({-1, l_input_size.back()});
  auto l_sorted     = std::get<0>(l_reshaped.sort(-1, /* descending=*/true));
  auto l_cumsum     = l_sorted.cumsum(-1);
  auto l_rhos       = torch::arange(1, l_sorted.size(-1) + 1, l_sorted.options()).unsqueeze(0);
  auto l_support    = (l_sorted * l_rhos) > (l_cumsum - 1);
  auto l_k          = l_support.sum(-1).unsqueeze(-1);
  auto l_tau_sum    = l_cumsum.gather(-1, l_k - 1);
  auto l_tau        = (l_tau_sum - 1) / l_k.to(l_tau_sum.dtype());
  auto l_output     = torch::clamp_min(l_reshaped - l_tau, 0);
  ctx->save_for_backward({l_output});
  return l_output.view(l_input_size);
}

torch::autograd::variable_list sparsemax::backward(
    torch::autograd::AutogradContext* ctx, torch::autograd::variable_list&& grad_outputs
) {
  auto saved        = ctx->get_saved_variables();
  auto l_output     = saved[0];
  auto l_grad       = grad_outputs[0];
  auto l_support    = (l_output > 0).to(l_grad.dtype());
  auto l_v_size     = l_output.size(-1);
  auto l_sum_grad   = (l_grad * l_support).sum(-1) / l_support.sum(-1).clamp_min(1e-20);
  auto l_grad_input = l_support * (l_grad - l_sum_grad.unsqueeze(-1));
  return {l_grad_input};
}
}  // namespace doodle::ai