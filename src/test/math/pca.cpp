//
// Created by TD on 2022/9/23.
//

#include <doodle_core/doodle_core.h>

#include <boost/test/unit_test.hpp>

#include "Eigen/Eigen"
#include <Eigen/src/Core/Matrix.h>
#include <Eigen/src/SVD/JacobiSVD.h>
#include <crtdbg.h>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <main_fixtures/lib_fixtures.h>

Eigen::MatrixXf pca_fun(Eigen::MatrixXf& in_mat) {
  // 计算平均
  Eigen::VectorXf Average = in_mat.colwise().mean();

  Eigen::VectorXf ones{in_mat.rows()};
  ones.setOnes();

  in_mat -= ones * Average.transpose();

  auto k = in_mat.rows() / 3;
  // svd 采样
  Eigen::JacobiSVD<Eigen::MatrixXf> svd{in_mat, Eigen::ComputeThinU | Eigen::ComputeThinV};
  Eigen::VectorXf EigenVector = svd.singularValues();
  std::cout << "The accuracy is " << EigenVector(k - 1) / EigenVector.sum() * 1.0 << std::endl;

  Eigen::MatrixXf ONB = svd.matrixV().block(0, k - 1, k, 1);
  std::cout << ONB;
  Eigen::VectorXf proj = ONB.transpose() * in_mat;
  return ONB * proj + Average;
};

BOOST_AUTO_TEST_CASE(test_pca) {
  Eigen::MatrixXf l_mat{2, 3};
  l_mat << 1, 2, 3, 4, 5, 6;
  pca_fun(l_mat);
}