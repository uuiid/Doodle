//
// Created by TD on 25-6-29.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/exception/exception.h>

#include <Eigen/Dense>
#include <cmath>
#include <cstdint>
#include <vector>

namespace doodle::ai {

/// @brief 非学习型正弦位置编码（对应 Python PositionalEncoding）
/// pe: [1, max_len, d_model] 的 sin/cos 编码表
class PositionalEncoding {
  Eigen::MatrixXf pe_;  // [max_len, d_model]
  std::int64_t d_model_{};
  std::int64_t max_len_{};

 public:
  PositionalEncoding() = default;

  /// @brief 初始化位置编码表
  /// @param d_model 特征维度
  /// @param max_len 最大序列长度
  void init(std::int64_t d_model, std::int64_t max_len = 5000) {
    d_model_ = d_model;
    max_len_ = max_len;
    pe_.resize(max_len, d_model);

    for (std::int64_t pos = 0; pos < max_len; ++pos) {
      for (std::int64_t i = 0; i < d_model / 2; ++i) {
        auto div_term = std::pow(10000.0, -2.0 * static_cast<double>(i) / static_cast<double>(d_model));
        auto sin_val = static_cast<float>(std::sin(static_cast<double>(pos) * div_term));
        auto cos_val = static_cast<float>(std::cos(static_cast<double>(pos) * div_term));

        if (2 * i < d_model) pe_(pos, 2 * i) = sin_val;
        if (2 * i + 1 < d_model) pe_(pos, 2 * i + 1) = cos_val;
      }
    }
    // 如果 d_model 为奇数，补充最后一列
    if (d_model % 2 == 1) {
      for (std::int64_t pos = 0; pos < max_len; ++pos) {
        pe_(pos, d_model - 1) = 0.0f;
      }
    }
  }

  /// @brief 对输入 [seq_len, d_model] 添加位置编码
  /// @param x [seq_len, d_model] 输入矩阵
  /// @return [seq_len, d_model]
  Eigen::MatrixXf forward(const Eigen::MatrixXf& x) const {
    DOODLE_CHICK(x.cols() == d_model_, "PositionalEncoding 维度不匹配: 期望 {}, 实际 {}", d_model_, x.cols());
    auto seq_len = x.rows();
    DOODLE_CHICK(seq_len <= max_len_, "序列长度 {} 超过最大长度 {}", seq_len, max_len_);
    return x + pe_.topRows(seq_len);
  }

  /// @brief 根据时间步索引提取位置编码 [indices, d_model]
  /// @param indices 时间步索引向量（每个值范围 [0, max_len)）
  /// @return [indices.size(), d_model]
  Eigen::MatrixXf lookup(const std::vector<std::int64_t>& indices) const {
    Eigen::MatrixXf result(static_cast<Eigen::Index>(indices.size()), d_model_);
    for (Eigen::Index i = 0; i < static_cast<Eigen::Index>(indices.size()); ++i) {
      result.row(i) = pe_.row(indices[static_cast<std::size_t>(i)]);
    }
    return result;
  }

  [[nodiscard]] std::int64_t d_model() const { return d_model_; }
  [[nodiscard]] std::int64_t max_len() const { return max_len_; }
  [[nodiscard]] const Eigen::MatrixXf& pe() const { return pe_; }
  [[nodiscard]] bool is_valid() const { return pe_.size() > 0; }
};

}  // namespace doodle::ai
