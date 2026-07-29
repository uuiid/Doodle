//
// Created by TD on 25-6-29.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/exception/exception.h>

#include <doodle_lib/ai/fwd.h>

#include <boost/numeric/conversion/cast.hpp>

#include <Eigen/Core>
#include <Eigen/Dense>
#include <cmath>
#include <cstdint>
#include <vector>


namespace doodle::ai {

/// @brief 非学习型正弦位置编码（对应 Python PositionalEncoding）
/// pe: [1, max_len, d_model] 的 sin/cos 编码表
class positional_encoding {
  MatrixXfRow pe_;  // [max_len, d_model]
  std::int64_t d_model_{};
  std::int64_t max_len_{};

 public:
  positional_encoding() = default;

  /// @brief 初始化位置编码表
  /// @param d_model 特征维度
  /// @param max_len 最大序列长度
  void init(std::int64_t d_model, std::int64_t max_len = 5000) {
    d_model_ = d_model;
    max_len_ = max_len;

    pe_.resize(max_len, d_model);

    const std::int64_t half_dim   = d_model / 2;

    // 位置向量 [max_len, 1]
    const Eigen::VectorXf pos_vec = Eigen::VectorXf::LinSpaced(max_len, 0.0f, boost::numeric_cast<float>(max_len - 1));

    // 频率向量 [half_dim, 1]
    Eigen::VectorXf div_term(half_dim);
    for (std::int64_t i = 0; i < half_dim; ++i) {
      div_term(i) = boost::numeric_cast<float>(
          std::pow(10000.0, -2.0 * boost::numeric_cast<double>(i) / boost::numeric_cast<double>(d_model))
      );
    }

    // angle = position * div_term^T  [max_len, 1] × [1, half_dim] = [max_len, half_dim]
    const MatrixXfRow angles = pos_vec * div_term.transpose();

    // 交替填充 sin(角度) 和 cos(角度) —— 向量化运算
    for (std::int64_t i = 0; i < half_dim; ++i) {
      pe_.col(2 * i)     = angles.col(i).array().sin();
      pe_.col(2 * i + 1) = angles.col(i).array().cos();
    }

    // d_model 为奇数时补充零列
    if (d_model % 2 == 1) {
      pe_.col(d_model - 1).setZero();
    }
  }

  /// @brief 对输入 [seq_len, d_model] 添加位置编码
  /// @param x [seq_len, d_model] 输入矩阵
  /// @return [seq_len, d_model]
  MatrixXfRow forward(const MatrixXfRow& x) const {
    DOODLE_CHICK(x.cols() == d_model_, "PositionalEncoding 维度不匹配: 期望 {}, 实际 {}", d_model_, x.cols());
    auto seq_len = x.rows();
    DOODLE_CHICK(seq_len <= max_len_, "序列长度 {} 超过最大长度 {}", seq_len, max_len_);
    return x + pe_.topRows(seq_len);
  }

  /// @brief 根据时间步索引提取位置编码 [indices, d_model]
  /// @param indices 时间步索引向量（每个值范围 [0, max_len)）
  /// @return [indices.size(), d_model]
  MatrixXfRow lookup(const std::vector<std::int64_t>& indices) const {
    // 检查索引范围
    for (const auto& idx : indices)
      DOODLE_CHICK(idx >= 0 && idx < max_len_, "PositionalEncoding 索引 {} 超出范围 [0, {})", idx, max_len_);
    return pe_(indices, Eigen::placeholders::all);
  }

  [[nodiscard]] std::int64_t d_model() const { return d_model_; }
  [[nodiscard]] std::int64_t max_len() const { return max_len_; }
  [[nodiscard]] const MatrixXfRow& pe() const { return pe_; }
  [[nodiscard]] bool is_valid() const { return pe_.size() > 0; }
};

}  // namespace doodle::ai
