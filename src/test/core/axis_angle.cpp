//
// Created by TD on 26-9-3.
//

#include <doodle_lib/ai/motion_rep/geometry.h>

#include <boost/test/unit_test.hpp>

#include <cmath>

namespace {

using doodle::ai::MatrixXfRow;

constexpr float kPi = 3.14159265358979323846f;

void check_close(float a, float b, float tol = 1e-4f) { BOOST_CHECK_SMALL(std::abs(a - b), tol); }

void check_matrix_close(const MatrixXfRow& a, const MatrixXfRow& b, float tol = 1e-4f) {
  BOOST_CHECK_EQUAL(a.rows(), b.rows());
  BOOST_CHECK_EQUAL(a.cols(), b.cols());
  if (a.rows() != b.rows() || a.cols() != b.cols()) return;
  for (Eigen::Index r = 0; r < a.rows(); ++r) {
    for (Eigen::Index c = 0; c < a.cols(); ++c) {
      check_close(a(r, c), b(r, c), tol);
    }
  }
}

}  // namespace

BOOST_AUTO_TEST_SUITE(geometry_axis_angle)

// axis_angle_to_matrix: 零轴角应得到单位矩阵
BOOST_AUTO_TEST_CASE(axis_angle_to_matrix_identity) {
  MatrixXfRow aa(1, 3);
  aa << 0.0f, 0.0f, 0.0f;

  MatrixXfRow expected(1, 9);
  expected << 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f;

  check_matrix_close(doodle::ai::axis_angle_to_matrix(aa), expected);
}

// axis_angle_to_matrix: 绕 z 轴旋转 90°
BOOST_AUTO_TEST_CASE(axis_angle_to_matrix_rot_z_90) {
  MatrixXfRow aa(1, 3);
  aa << 0.0f, 0.0f, kPi / 2.0f;

  MatrixXfRow expected(1, 9);
  expected << 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f;

  check_matrix_close(doodle::ai::axis_angle_to_matrix(aa), expected);
}

// axis_angle_to_matrix: 绕 x 轴旋转 90°
BOOST_AUTO_TEST_CASE(axis_angle_to_matrix_rot_x_90) {
  MatrixXfRow aa(1, 3);
  aa << kPi / 2.0f, 0.0f, 0.0f;

  MatrixXfRow expected(1, 9);
  expected << 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f;

  check_matrix_close(doodle::ai::axis_angle_to_matrix(aa), expected);
}

// matrix_to_axis_angle: 单位矩阵应得到零轴角
BOOST_AUTO_TEST_CASE(matrix_to_axis_angle_identity) {
  MatrixXfRow m(1, 9);
  m << 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f;

  MatrixXfRow expected(1, 3);
  expected << 0.0f, 0.0f, 0.0f;

  check_matrix_close(doodle::ai::matrix_to_axis_angle(m), expected);
}

// matrix_to_axis_angle: 绕 z 轴旋转 90° 的矩阵
BOOST_AUTO_TEST_CASE(matrix_to_axis_angle_rot_z_90) {
  MatrixXfRow m(1, 9);
  m << 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f;

  MatrixXfRow expected(1, 3);
  expected << 0.0f, 0.0f, kPi / 2.0f;

  check_matrix_close(doodle::ai::matrix_to_axis_angle(m), expected);
}

// 往返测试: axis_angle -> matrix -> axis_angle
BOOST_AUTO_TEST_CASE(axis_angle_round_trip) {
  MatrixXfRow aa(1, 3);
  aa << 0.5f, -0.3f, 0.2f;

  MatrixXfRow back = doodle::ai::matrix_to_axis_angle(doodle::ai::axis_angle_to_matrix(aa));

  check_matrix_close(back, aa, 1e-3f);
}

// 往返测试: matrix -> axis_angle -> matrix
BOOST_AUTO_TEST_CASE(matrix_round_trip) {
  MatrixXfRow aa(1, 3);
  aa << 0.5f, -0.3f, 0.2f;

  MatrixXfRow m    = doodle::ai::axis_angle_to_matrix(aa);
  MatrixXfRow back = doodle::ai::axis_angle_to_matrix(doodle::ai::matrix_to_axis_angle(m));

  check_matrix_close(back, m, 1e-3f);
}

BOOST_AUTO_TEST_SUITE_END()
