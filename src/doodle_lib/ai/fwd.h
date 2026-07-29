#pragma once

#include <Eigen/Eigen>

namespace doodle::ai {
// 定义行主序矩阵
using MatrixXfRow = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using MatrixXbRow = Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

}  // namespace doodle::ai