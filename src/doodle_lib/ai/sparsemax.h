#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <filesystem>
#include <memory>
#include <torch/csrc/api/include/torch/torch.h>
#include <torch/csrc/autograd/custom_function.h>
#include <torch/csrc/autograd/function.h>
#include <torch/serialize/input-archive.h>
#include <vector>

namespace doodle::ai {
struct sparsemax : torch::autograd::Function<sparsemax> {
  static torch::Tensor forward(torch::autograd::AutogradContext* ctx, const torch::Tensor& input, int dim = -1);
  static torch::autograd::variable_list backward(
      torch::autograd::AutogradContext* ctx, torch::autograd::variable_list grad_outputs
  );
};
}  // namespace doodle::ai