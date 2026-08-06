#pragma once

#include <Eigen/Eigen>

namespace doodle::ai {
// 定义行主序矩阵
using MatrixXfRow  = Eigen::Matrix<std::float_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using MatrixXdRow  = Eigen::Matrix<std::double_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using MatrixXbRow  = Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

using MatrixX3fRow = Eigen::Matrix<std::float_t, Eigen::Dynamic, 3, Eigen::RowMajor>;
using MatrixX2fRow = Eigen::Matrix<std::float_t, Eigen::Dynamic, 2, Eigen::RowMajor>;
using MatrixX4bRow = Eigen::Matrix<bool, Eigen::Dynamic, 4, Eigen::RowMajor>;

}  // namespace doodle::ai