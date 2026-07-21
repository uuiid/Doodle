//
// Created by TD on 25-7-21.
//
#include "geometry.h"

#include <doodle_core/exception/exception.h>

namespace doodle::ai {

Eigen::MatrixXf matrix_to_cont6d(const Eigen::MatrixXf& matrix) {
  // matrix: [N, 9], each row is a 3x3 matrix flattened row-major
  const Eigen::Index N = matrix.rows();
  DOODLE_CHICK(matrix.cols() == 9, "matrix_to_cont6d: input cols must be 9, got {}", matrix.cols());

  Eigen::MatrixXf cont6d(N, 6);
  for (Eigen::Index i = 0; i < N; ++i) {
    // First two columns of the rotation matrix
    // Row-major: indices 0,1,2 = col0, 3,4,5 = col1, 6,7,8 = col2
    cont6d(i, 0) = matrix(i, 0);  // col0.x
    cont6d(i, 1) = matrix(i, 3);  // col0.y
    cont6d(i, 2) = matrix(i, 6);  // col0.z
    cont6d(i, 3) = matrix(i, 1);  // col1.x
    cont6d(i, 4) = matrix(i, 4);  // col1.y
    cont6d(i, 5) = matrix(i, 7);  // col1.z
  }
  return cont6d;
}

Eigen::MatrixXf cont6d_to_matrix(const Eigen::MatrixXf& cont6d) {
  const Eigen::Index N = cont6d.rows();
  DOODLE_CHICK(cont6d.cols() == 6, "cont6d_to_matrix: input cols must be 6, got {}", cont6d.cols());

  Eigen::MatrixXf matrix(N, 9);

  for (Eigen::Index i = 0; i < N; ++i) {
    // Extract first two columns from 6D representation
    Eigen::Vector3f x_raw(cont6d(i, 0), cont6d(i, 1), cont6d(i, 2));
    Eigen::Vector3f y_raw(cont6d(i, 3), cont6d(i, 4), cont6d(i, 5));

    // Gram-Schmidt orthogonalization
    // x = normalize(x_raw)
    Eigen::Vector3f x = x_raw.normalized();

    // z = normalize(cross(x, y_raw))
    Eigen::Vector3f z = x.cross(y_raw).normalized();

    // y = cross(z, x)
    Eigen::Vector3f y = z.cross(x);

    // Build rotation matrix (column-major storage but we store row-major)
    // Matrix R = [x | y | z]  (columns are x, y, z)
    // Row-major storage: row0 = [x0, y0, z0], row1 = [x1, y1, z1], row2 = [x2, y2, z2]
    matrix(i, 0) = x(0);  matrix(i, 1) = y(0);  matrix(i, 2) = z(0);
    matrix(i, 3) = x(1);  matrix(i, 4) = y(1);  matrix(i, 5) = z(1);
    matrix(i, 6) = x(2);  matrix(i, 7) = y(2);  matrix(i, 8) = z(2);
  }

  return matrix;
}

Eigen::Matrix3f angle_to_Y_rotation_matrix(float angle) {
  const float cos_a = std::cos(angle);
  const float sin_a = std::sin(angle);

  Eigen::Matrix3f mat;
  mat(0, 0) = cos_a;  mat(0, 1) = 0.0f; mat(0, 2) = sin_a;
  mat(1, 0) = 0.0f;   mat(1, 1) = 1.0f; mat(1, 2) = 0.0f;
  mat(2, 0) = -sin_a; mat(2, 1) = 0.0f; mat(2, 2) = cos_a;

  return mat;
}

Eigen::MatrixXf angle_to_Y_rotation_matrix_batch(const Eigen::VectorXf& angles) {
  const Eigen::Index N = angles.size();
  Eigen::MatrixXf result(N, 9);

  for (Eigen::Index i = 0; i < N; ++i) {
    Eigen::Matrix3f R = angle_to_Y_rotation_matrix(angles(i));
    result(i, 0) = R(0, 0); result(i, 1) = R(0, 1); result(i, 2) = R(0, 2);
    result(i, 3) = R(1, 0); result(i, 4) = R(1, 1); result(i, 5) = R(1, 2);
    result(i, 6) = R(2, 0); result(i, 7) = R(2, 1); result(i, 8) = R(2, 2);
  }

  return result;
}

}  // namespace doodle::ai
