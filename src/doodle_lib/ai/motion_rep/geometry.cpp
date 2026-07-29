//
// Created by TD on 25-7-21.
//
#include "geometry.h"

#include <doodle_core/exception/exception.h>

namespace doodle::ai {

MatrixXfRow matrix_to_cont6d(const MatrixXfRow& matrix) {
  // matrix: [N, J*9], each group of 9 cols is a 3x3 matrix flattened row-major
  const Eigen::Index N = matrix.rows();
  const Eigen::Index C = matrix.cols();
  DOODLE_CHICK(C % 9 == 0, "matrix_to_cont6d: input cols must be multiple of 9, got {}", C);
  const Eigen::Index J = C / 9;  // number of joints

  MatrixXfRow cont6d(N, J * 6);
  for (Eigen::Index j = 0; j < J; ++j) {
    const Eigen::Index col_offset = j * 9;
    const Eigen::Index out_offset = j * 6;
    for (Eigen::Index i = 0; i < N; ++i) {
      // First two columns of the rotation matrix
      // Row-major: indices 0,1,2 = col0, 3,4,5 = col1, 6,7,8 = col2
      cont6d(i, out_offset + 0) = matrix(i, col_offset + 0);  // col0.x
      cont6d(i, out_offset + 1) = matrix(i, col_offset + 3);  // col0.y
      cont6d(i, out_offset + 2) = matrix(i, col_offset + 6);  // col0.z
      cont6d(i, out_offset + 3) = matrix(i, col_offset + 1);  // col1.x
      cont6d(i, out_offset + 4) = matrix(i, col_offset + 4);  // col1.y
      cont6d(i, out_offset + 5) = matrix(i, col_offset + 7);  // col1.z
    }
  }
  return cont6d;
}

MatrixXfRow cont6d_to_matrix(const MatrixXfRow& cont6d) {
  const Eigen::Index N = cont6d.rows();
  const Eigen::Index C = cont6d.cols();
  DOODLE_CHICK(C % 6 == 0, "cont6d_to_matrix: input cols must be multiple of 6, got {}", C);
  const Eigen::Index J = C / 6;  // number of joints

  MatrixXfRow matrix(N, J * 9);

  for (Eigen::Index j = 0; j < J; ++j) {
    const Eigen::Index col_offset = j * 6;
    const Eigen::Index out_offset = j * 9;
    for (Eigen::Index i = 0; i < N; ++i) {
      // Extract first two columns from 6D representation
      Eigen::Vector3f x_raw(cont6d(i, col_offset + 0), cont6d(i, col_offset + 1), cont6d(i, col_offset + 2));
      Eigen::Vector3f y_raw(cont6d(i, col_offset + 3), cont6d(i, col_offset + 4), cont6d(i, col_offset + 5));

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
      matrix(i, out_offset + 0) = x(0);
      matrix(i, out_offset + 1) = y(0);
      matrix(i, out_offset + 2) = z(0);
      matrix(i, out_offset + 3) = x(1);
      matrix(i, out_offset + 4) = y(1);
      matrix(i, out_offset + 5) = z(1);
      matrix(i, out_offset + 6) = x(2);
      matrix(i, out_offset + 7) = y(2);
      matrix(i, out_offset + 8) = z(2);
    }
  }

  return matrix;
}

Eigen::Matrix3f angle_to_Y_rotation_matrix(float angle) {
  const float cos_a = std::cos(angle);
  const float sin_a = std::sin(angle);

  Eigen::Matrix3f mat;
  mat(0, 0) = cos_a;
  mat(0, 1) = 0.0f;
  mat(0, 2) = sin_a;
  mat(1, 0) = 0.0f;
  mat(1, 1) = 1.0f;
  mat(1, 2) = 0.0f;
  mat(2, 0) = -sin_a;
  mat(2, 1) = 0.0f;
  mat(2, 2) = cos_a;

  return mat;
}

MatrixXfRow angle_to_Y_rotation_matrix_batch(const Eigen::VectorXf& angles) {
  const Eigen::Index N = angles.size();
  MatrixXfRow result(N, 9);

  for (Eigen::Index i = 0; i < N; ++i) {
    Eigen::Matrix3f R = angle_to_Y_rotation_matrix(angles(i));
    result(i, 0)      = R(0, 0);
    result(i, 1)      = R(0, 1);
    result(i, 2)      = R(0, 2);
    result(i, 3)      = R(1, 0);
    result(i, 4)      = R(1, 1);
    result(i, 5)      = R(1, 2);
    result(i, 6)      = R(2, 0);
    result(i, 7)      = R(2, 1);
    result(i, 8)      = R(2, 2);
  }

  return result;
}

// ======================================================================
// axis_angle_to_matrix: Rodrigues 公式
// ======================================================================
MatrixXfRow axis_angle_to_matrix(const MatrixXfRow& axis_angle) {
  const Eigen::Index N = axis_angle.rows();
  DOODLE_CHICK(axis_angle.cols() == 3, "axis_angle_to_matrix: input cols must be 3, got {}", axis_angle.cols());

  constexpr float eps = 1e-6f;
  MatrixXfRow result(N, 9);

  for (Eigen::Index i = 0; i < N; ++i) {
    const Eigen::Vector3f v = axis_angle.row(i);
    const float angle       = v.norm();
    Eigen::Vector3f axis;
    if (angle < eps) {
      // 接近零角度：近似 R ≈ I + [v]×
      axis = Eigen::Vector3f::Zero();
      // 直接用小角度近似：R ≈ I + skew(v)
      const float vx = v(0), vy = v(1), vz = v(2);
      // clang-format off
      result.row(i) <<
          1.0f, -vz,  vy,
          vz,  1.0f, -vx,
         -vy,  vx,  1.0f;
      // clang-format on
      continue;
    }
    axis        = v / angle;

    const float c = std::cos(angle);
    const float s = std::sin(angle);

    // 斜对称矩阵 K = [0, -z, y; z, 0, -x; -y, x, 0]
    const float x = axis(0), y = axis(1), z = axis(2);

    // Rodrigues: R = I + sin(θ) * K + (1 - cos(θ)) * K²
    // K² 的解析形式
    const float Ksq_00 = -y * y - z * z;
    const float Ksq_01 = x * y;
    const float Ksq_02 = x * z;
    const float Ksq_10 = x * y;
    const float Ksq_11 = -x * x - z * z;
    const float Ksq_12 = y * z;
    const float Ksq_20 = x * z;
    const float Ksq_21 = y * z;
    const float Ksq_22 = -x * x - y * y;

    const float omc = 1.0f - c;

    // clang-format off
    result.row(i) <<
        c + omc * Ksq_00,  -z * s + omc * Ksq_01,   y * s + omc * Ksq_02,
        z * s + omc * Ksq_10,  c + omc * Ksq_11,  -x * s + omc * Ksq_12,
       -y * s + omc * Ksq_20,   x * s + omc * Ksq_21,  c + omc * Ksq_22;
    // clang-format on
  }
  return result;
}

// ======================================================================
// matrix_to_quaternion: 旋转矩阵 → 四元数
// ======================================================================
MatrixXfRow matrix_to_quaternion(const MatrixXfRow& matrix) {
  const Eigen::Index N = matrix.rows();
  DOODLE_CHICK(matrix.cols() == 9, "matrix_to_quaternion: input cols must be 9, got {}", matrix.cols());

  MatrixXfRow quat(N, 4);

  for (Eigen::Index i = 0; i < N; ++i) {
    // Row-major storage: R = [r00,r01,r02; r10,r11,r12; r20,r21,r22]
    const float r00 = matrix(i, 0), r01 = matrix(i, 1), r02 = matrix(i, 2);
    const float r10 = matrix(i, 3), r11 = matrix(i, 4), r12 = matrix(i, 5);
    const float r20 = matrix(i, 6), r21 = matrix(i, 7), r22 = matrix(i, 8);

    const float tr = r00 + r11 + r22;
    float w, x, y, z;

    if (tr > 0.0f) {
      float s = std::sqrt(tr + 1.0f) * 2.0f;  // S = 4 * qw
      w       = 0.25f * s;
      x       = (r21 - r12) / s;
      y       = (r02 - r20) / s;
      z       = (r10 - r01) / s;
    } else if (r00 > r11 && r00 > r22) {
      float s = std::sqrt(1.0f + r00 - r11 - r22) * 2.0f;  // S = 4 * qx
      w       = (r21 - r12) / s;
      x       = 0.25f * s;
      y       = (r01 + r10) / s;
      z       = (r02 + r20) / s;
    } else if (r11 > r22) {
      float s = std::sqrt(1.0f + r11 - r00 - r22) * 2.0f;  // S = 4 * qy
      w       = (r02 - r20) / s;
      x       = (r01 + r10) / s;
      y       = 0.25f * s;
      z       = (r12 + r21) / s;
    } else {
      float s = std::sqrt(1.0f + r22 - r00 - r11) * 2.0f;  // S = 4 * qz
      w       = (r10 - r01) / s;
      x       = (r02 + r20) / s;
      y       = (r12 + r21) / s;
      z       = 0.25f * s;
    }

    quat.row(i) << w, x, y, z;
  }
  return quat;
}

// ======================================================================
// quaternion_to_axis_angle: 四元数 → 轴角
// ======================================================================
MatrixXfRow quaternion_to_axis_angle(const MatrixXfRow& quat) {
  const Eigen::Index N = quat.rows();
  DOODLE_CHICK(quat.cols() == 4, "quaternion_to_axis_angle: input cols must be 4, got {}", quat.cols());

  constexpr float eps = 1e-6f;
  MatrixXfRow result(N, 3);

  for (Eigen::Index i = 0; i < N; ++i) {
    float w  = quat(i, 0);
    float x  = quat(i, 1);
    float y  = quat(i, 2);
    float z  = quat(i, 3);

    // 标准化到规范形式：优先 w > 0；当 w ≈ 0 时优先第一个非零分量 > 0
    if (w < -eps || (std::abs(w) <= eps && x < 0.0f)) {
      w = -w;
      x = -x;
      y = -y;
      z = -z;
    }

    const float sin_half_angle = std::sqrt(x * x + y * y + z * z);
    const float angle          = 2.0f * std::atan2(sin_half_angle, w);

    if (sin_half_angle < eps) {
      // 小角度近似：axis_angle ≈ 2 * xyz
      result.row(i) << 2.0f * x, 2.0f * y, 2.0f * z;
    } else {
      const float scale = angle / sin_half_angle;
      result.row(i) << x * scale, y * scale, z * scale;
    }
  }
  return result;
}

// ======================================================================
// matrix_to_axis_angle: 旋转矩阵 → 轴角（通过四元数）
// ======================================================================
MatrixXfRow matrix_to_axis_angle(const MatrixXfRow& matrix) {
  const auto quat = matrix_to_quaternion(matrix);
  return quaternion_to_axis_angle(quat);
}

}  // namespace doodle::ai
