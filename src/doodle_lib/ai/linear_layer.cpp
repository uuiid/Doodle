//
// Created by TD on 25-6-29.
//
#include "linear_layer.h"

#include <doodle_core/exception/exception.h>

#include <cnpy.h>

namespace doodle::ai {

void LinearLayer::load(const FSys::path& weight_path, const FSys::path& bias_path) {
  // 加载权重
  auto l_data = cnpy::npy_load(weight_path.string());
  DOODLE_CHICK(l_data.shape.size() >= 2, "LinearLayer 权重 shape 维度不足: {}", weight_path.string());
  std::int64_t rows = l_data.shape[0];
  std::int64_t cols = l_data.shape[1];

  // npy 以行主序存储（C order），Eigen 默认为列主序
  // 直接使用 RowMajor 映射避免转置
  Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> w_map(
      l_data.data<float>(), rows, cols
  );
  weight_ = w_map;

  // 加载偏置（可选）
  if (!bias_path.empty()) {
    auto b_data = cnpy::npy_load(bias_path.string());
    DOODLE_CHICK(b_data.shape.size() == 1, "LinearLayer 偏置 shape 应为 1D: {}", bias_path.string());
    DOODLE_CHICK(
        static_cast<std::int64_t>(b_data.shape[0]) == rows, "LinearLayer 偏置大小 {} 不匹配权重行数 {}: {}", b_data.shape[0],
        rows, bias_path.string()
    );
    Eigen::Map<Eigen::VectorXf> b_map(b_data.data<float>(), static_cast<Eigen::Index>(b_data.shape[0]));
    bias_ = b_map;
  }
}

Eigen::MatrixXf LinearLayer::forward(const Eigen::MatrixXf& input) const {
  DOODLE_CHICK(is_valid(), "LinearLayer 未初始化");
  DOODLE_CHICK(
      input.cols() == in_features(), "LinearLayer 输入特征维度不匹配: 期望 {}, 实际 {}", in_features(), input.cols()
  );

  // y = x * W^T + b
  // input: [N, in_features], weight_: [out_features, in_features]
  // result: [N, out_features]
  Eigen::MatrixXf result = input * weight_.transpose();
  if (has_bias()) {
    result.rowwise() += bias_.transpose();
  }
  return result;
}

Eigen::MatrixXf LinearLayer::forward_batched(
    const Eigen::MatrixXf& input, std::int64_t batch_size, std::int64_t time_steps
) const {
  // input: [B*T, in_features]
  DOODLE_CHICK(is_valid(), "LinearLayer 未初始化");
  DOODLE_CHICK(
      input.cols() == in_features(), "LinearLayer 输入特征维度不匹配: 期望 {}, 实际 {}", in_features(), input.cols()
  );
  DOODLE_CHICK(
      input.rows() == batch_size * time_steps, "LinearLayer 批大小不匹配: 期望 {}*{}={}, 实际 {}", batch_size,
      time_steps, batch_size * time_steps, input.rows()
  );

  return forward(input);
}

}  // namespace doodle::ai
