//
// Created by TD on 2022/9/23.
//

#include <doodle_core/doodle_core.h>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include "torch/torch.h"
#include <ATen/core/TensorBody.h>
#include <ATen/ops/exp.h>
#include <iostream>
#include <ostream>
#include <torch/csrc/autograd/generated/variable_factories.h>
#include <torch/custom_class_detail.h>
#include <torch/nn/module.h>

BOOST_AUTO_TEST_CASE(ai_base) { std::cout << torch::eye(3) << std::endl; }

using namespace std::literals;

class network : public torch::nn::Module {
 public:
  torch::Tensor w_, b_;
  network(std::int32_t in_n, std::int32_t in_m) : w_(), b_() {
    w_ = register_parameter("w_"s, torch::randn({in_n, in_m}));
    b_ = register_parameter("b_"s, torch::randn(in_m));
  }

 private:
  /**
   * @brief S 状神经元
   *
   * @param in_tensor 传入权重
   * @return torch::Tensor
   */
  static torch::Tensor sigmoid(const at::Tensor &in_tensor) { return 1 / (1.0 + torch::exp(in_tensor)); }
  /**
   * @brief S状神经元导数
   *
   * @param in_tensor
   * @return torch::Tensor
   */
  static torch::Tensor sigmoid_prime(const at::Tensor &in_tensor) {
    return sigmoid(in_tensor) * (1 - sigmoid(in_tensor));
  };
};