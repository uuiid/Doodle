//
// Created by TD on 25-6-29.
//
#include "linear_layer.h"

#include <doodle_core/exception/exception.h>

#include <cnpy.h>

namespace doodle::ai {

void linear_layer::load(const FSys::path& weight_path, const FSys::path& bias_path) {
  DOODLE_CHICK(FSys::exists(weight_path), "linear_layer 权重文件不存在: {}", weight_path.string());

  // 加载权重
  auto l_data = cnpy::npy_load(weight_path.string());
  DOODLE_CHICK(l_data.word_size == sizeof(float), "linear_layer 权重数据类型不是 float32: {}", weight_path.string());
  DOODLE_CHICK(l_data.shape.size() >= 2, "linear_layer 权重 shape 维度不足: {}", weight_path.string());
  std::int64_t rows = l_data.shape[0];
  std::int64_t cols = l_data.shape[1];

  DOODLE_CHICK(l_data.word_size == sizeof(float), "linear_layer 权重数据类型不是 float32: {}", weight_path.string());
  DOODLE_CHICK(l_data.fortran_order == false, "linear_layer 权重不是 C order (row-major): {}", weight_path.string());

  // npy 以行主序存储（C order），Eigen 默认为列主序
  // 直接使用 RowMajor 映射避免转置
  Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> w_map(
      l_data.data<float>(), rows, cols
  );
  weight_   = w_map;
  weight_T_ = weight_.transpose();  // 预转置，forward 时直接用 input * weight_T_

  // 加载偏置（可选）
  if (!bias_path.empty()) {
    auto b_data = cnpy::npy_load(bias_path.string());
    DOODLE_CHICK(b_data.word_size == sizeof(float), "linear_layer 偏置数据类型不是 float32: {}", bias_path.string());
    DOODLE_CHICK(b_data.shape.size() == 1, "linear_layer 偏置 shape 应为 1D: {}", bias_path.string());
    DOODLE_CHICK(
        static_cast<std::int64_t>(b_data.shape[0]) == rows, "linear_layer 偏置大小 {} 不匹配权重行数 {}: {}",
        b_data.shape[0], rows, bias_path.string()
    );
    Eigen::Map<Eigen::VectorXf> b_map(b_data.data<float>(), static_cast<Eigen::Index>(b_data.shape[0]));
    bias_ = b_map;
  }
}

MatrixXfRow linear_layer::forward(const MatrixXfRow& input) const {
  DOODLE_CHICK(is_valid(), "linear_layer 未初始化");
  DOODLE_CHICK(
      input.cols() == in_features(), "linear_layer 输入特征维度不匹配: 期望 {}, 实际 {}", in_features(), input.cols()
  );

  // y = x * W^T + b
  // input: [N, in_features], weight_T_: [in_features, out_features]
  // result: [N, out_features]
  // noalias 告知 Eigen 输出不别名输入，可走优化 GEMM 内核
  MatrixXfRow result(input.rows(), weight_T_.cols());
  result.noalias() = input * weight_T_;
  if (has_bias()) {
    result.rowwise() += bias_.transpose();
  }
  return result;
}

MatrixXfRow linear_layer::forward_batched(
    const MatrixXfRow& input, std::int64_t batch_size, std::int64_t time_steps
) const {
  // input: [B*T, in_features]
  DOODLE_CHICK(is_valid(), "linear_layer 未初始化");
  DOODLE_CHICK(
      input.cols() == in_features(), "linear_layer 输入特征维度不匹配: 期望 {}, 实际 {}", in_features(), input.cols()
  );
  DOODLE_CHICK(
      input.rows() == batch_size * time_steps, "linear_layer 批大小不匹配: 期望 {}*{}={}, 实际 {}", batch_size,
      time_steps, batch_size * time_steps, input.rows()
  );

  return forward(input);
}

}  // namespace doodle::ai
