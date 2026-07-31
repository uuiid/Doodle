//
// Created by TD on 25-7-21.
//
#include "feature_utils.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/ai/motion_rep/geometry.h>

#include <cmath>
#include <vector>

namespace doodle::ai {

// ======================================================================
// diff_angles: 帧间角度差
// ======================================================================
MatrixXfRow diff_angles(const MatrixXfRow& angles, float fps) {
  const Eigen::Index B = angles.rows();
  const Eigen::Index T = angles.cols();

  if (T <= 1) {
    // 不足两帧，返回全零
    return MatrixXfRow::Zero(B, T);
  }

  // diff = fps * atan2(sin_diff, cos_diff)
  MatrixXfRow result(B, T);
  result.setZero();

  for (Eigen::Index b = 0; b < B; ++b) {
    for (Eigen::Index t = 0; t < T - 1; ++t) {
      const float cos_t    = std::cos(angles(b, t));
      const float sin_t    = std::sin(angles(b, t));
      const float cos_t1   = std::cos(angles(b, t + 1));
      const float sin_t1   = std::sin(angles(b, t + 1));

      const float cos_diff = cos_t1 * cos_t + sin_t1 * sin_t;
      const float sin_diff = sin_t1 * cos_t - cos_t1 * sin_t;

      result(b, t)         = fps * std::atan2(sin_diff, cos_diff);
    }
    // 最后一帧使用前一帧的值（重复 padding）
    if (T >= 2) {
      result(b, T - 1) = result(b, T - 2);
    }
  }

  return result;
}

// ======================================================================
// compute_heading_angle: 从关节位置计算朝向角
// ======================================================================
MatrixXfRow compute_heading_angle(
    const MatrixXfRow& posed_joints, const skeleton_base& skel, std::int64_t batch_size, std::int64_t time_steps
) {
  const Eigen::Index total = posed_joints.rows();
  const Eigen::Index J     = skel.nbjoints_;
  DOODLE_CHICK(total == batch_size * time_steps, "总帧数不匹配");
  DOODLE_CHICK(posed_joints.cols() == J * 3, "列数不匹配 J*3");
  DOODLE_CHICK(skel.hip_joint_idx_.size() >= 2, "需要至少 2 个髋关节索引");

  const std::int64_t r_hip = skel.hip_joint_idx_[0];  // right hip
  const std::int64_t l_hip = skel.hip_joint_idx_[1];  // left hip

  MatrixXfRow heading(batch_size, time_steps);

  for (Eigen::Index b = 0; b < batch_size; ++b) {
    for (Eigen::Index t = 0; t < time_steps; ++t) {
      const Eigen::Index idx = b * time_steps + t;

      // 右髋和左髋位置
      const float r_x        = posed_joints(idx, r_hip * 3 + 0);
      const float r_z        = posed_joints(idx, r_hip * 3 + 2);
      const float l_x        = posed_joints(idx, l_hip * 3 + 0);
      const float l_z        = posed_joints(idx, l_hip * 3 + 2);

      // diff = right_hip - left_hip
      // heading = atan2(diff.z, -diff.x)
      heading(b, t)          = std::atan2(r_z - l_z, -(r_x - l_x));
    }
  }

  return heading;
}

// ======================================================================
// compute_vel_xyz: 关节速度
// ======================================================================
MatrixXfRow compute_vel_xyz(
    const MatrixXfRow& positions, float fps, std::int64_t batch_size, std::int64_t time_steps, std::int64_t nbjoints,
    const Eigen::VectorXi& lengths
) {
  const Eigen::Index total = positions.rows();
  DOODLE_CHICK(total == batch_size * time_steps, "总帧数不匹配");
  DOODLE_CHICK(positions.cols() == nbjoints * 3, "列数不匹配 J*3");

  // velocity = fps * (positions[t+1] - positions[t])
  // 最后一帧复制前一帧的值
  MatrixXfRow velocity(total, nbjoints * 3);
  velocity.setZero();

  for (Eigen::Index b = 0; b < batch_size; ++b) {
    const std::int64_t len = (lengths.size() > 0) ? lengths(b) : time_steps;

    for (Eigen::Index t = 0; t < len - 1; ++t) {
      const Eigen::Index idx   = b * time_steps + t;
      const Eigen::Index idx_n = b * time_steps + t + 1;

      for (Eigen::Index j = 0; j < nbjoints; ++j) {
        velocity(idx, j * 3 + 0) = fps * (positions(idx_n, j * 3 + 0) - positions(idx, j * 3 + 0));
        velocity(idx, j * 3 + 1) = fps * (positions(idx_n, j * 3 + 1) - positions(idx, j * 3 + 1));
        velocity(idx, j * 3 + 2) = fps * (positions(idx_n, j * 3 + 2) - positions(idx, j * 3 + 2));
      }
    }

    // 最后一帧：复制前一帧的速度
    if (len >= 2) {
      const Eigen::Index last = b * time_steps + len - 1;
      const Eigen::Index prev = b * time_steps + len - 2;
      velocity.row(last)      = velocity.row(prev);
    }
  }

  return velocity;
}

// ======================================================================
// compute_vel_angle: 局部根节点旋转速度
// ======================================================================
MatrixXfRow compute_vel_angle(const MatrixXfRow& root_rot_angles, float fps, const Eigen::VectorXi& lengths) {
  // root_rot_angles: [B, T], 输出: [B, T]
  return diff_angles(root_rot_angles, fps);
}

// ======================================================================
// foot_detect_from_pos_and_vel: 脚接触检测
// ======================================================================
MatrixXbRow foot_detect_from_pos_and_vel(
    const MatrixXfRow& positions, const MatrixXfRow& velocity, const skeleton_base& skel, std::int64_t batch_size,
    std::int64_t time_steps, float vel_thres, float height_thresh
) {
  const Eigen::Index total    = positions.rows();
  const Eigen::Index J        = skel.nbjoints_;

  // 最多使用每侧 2 个脚关节
  const auto fid_l            = skel.left_foot_joint_idx_;
  const auto fid_r            = skel.right_foot_joint_idx_;
  const std::int64_t n_foot_l = (std::min)(static_cast<std::int64_t>(fid_l.size()), std::int64_t{2});
  const std::int64_t n_foot_r = (std::min)(static_cast<std::int64_t>(fid_r.size()), std::int64_t{2});

  MatrixXbRow contacts(total, 4);
  contacts.setConstant(false);

  for (Eigen::Index b = 0; b < batch_size; ++b) {
    for (Eigen::Index t = 0; t < time_steps; ++t) {
      const Eigen::Index idx = b * time_steps + t;

      // 左脚
      for (std::int64_t f = 0; f < n_foot_l && f < 2; ++f) {
        const auto j_idx = fid_l[static_cast<std::size_t>(f)];
        const float v    = std::sqrt(
            velocity(idx, j_idx * 3 + 0) * velocity(idx, j_idx * 3 + 0) +
            velocity(idx, j_idx * 3 + 1) * velocity(idx, j_idx * 3 + 1) +
            velocity(idx, j_idx * 3 + 2) * velocity(idx, j_idx * 3 + 2)
        );
        const float h    = positions(idx, j_idx * 3 + 1);  // Y 轴高度
        contacts(idx, f) = (v < vel_thres) && (h < height_thresh);
      }

      // 右脚
      for (std::int64_t f = 0; f < n_foot_r && f < 2; ++f) {
        const auto j_idx = fid_r[static_cast<std::size_t>(f)];
        const float v    = std::sqrt(
            velocity(idx, j_idx * 3 + 0) * velocity(idx, j_idx * 3 + 0) +
            velocity(idx, j_idx * 3 + 1) * velocity(idx, j_idx * 3 + 1) +
            velocity(idx, j_idx * 3 + 2) * velocity(idx, j_idx * 3 + 2)
        );
        const float h        = positions(idx, j_idx * 3 + 1);
        contacts(idx, 2 + f) = (v < vel_thres) && (h < height_thresh);
      }
    }
  }

  return contacts;
}

// ======================================================================
// length_to_mask: 长度 → 布尔掩码
// ======================================================================
MatrixXbRow length_to_mask(const Eigen::VectorXi& lengths, std::int64_t max_len) {
  const Eigen::Index B = lengths.size();
  if (max_len <= 0) {
    max_len = lengths.maxCoeff();
  }

  MatrixXbRow mask(B, max_len);
  for (Eigen::Index b = 0; b < B; ++b) {
    for (Eigen::Index t = 0; t < max_len; ++t) {
      mask(b, t) = (t < lengths(b));
    }
  }
  return mask;
}

// ======================================================================
// get_smooth_root_pos: 平滑根位置（移动平均）
// ======================================================================
MatrixXfRow get_smooth_root_pos(
    const MatrixXfRow& root_positions, std::int64_t batch_size, std::int64_t time_steps, std::int64_t window_size
) {
  const Eigen::Index total = root_positions.rows();
  DOODLE_CHICK(total == batch_size * time_steps, "总帧数不匹配");
  DOODLE_CHICK(root_positions.cols() == 3, "列数 != 3");

  MatrixXfRow smooth(total, 3);

  for (Eigen::Index b = 0; b < batch_size; ++b) {
    for (Eigen::Index t = 0; t < time_steps; ++t) {
      // 计算移动平均窗口
      std::int64_t half_win = window_size / 2;
      std::int64_t start    = (std::max)(std::int64_t{0}, t - half_win);
      std::int64_t end      = (std::min)(time_steps - 1, t + half_win);
      std::int64_t count    = end - start + 1;

      float sum_x = 0, sum_y = 0, sum_z = 0;
      for (std::int64_t s = start; s <= end; ++s) {
        const Eigen::Index idx = b * time_steps + s;
        sum_x += root_positions(idx, 0);
        sum_y += root_positions(idx, 1);
        sum_z += root_positions(idx, 2);
      }

      const Eigen::Index out_idx = b * time_steps + t;
      smooth(out_idx, 0)         = sum_x / static_cast<float>(count);
      smooth(out_idx, 1)         = sum_y / static_cast<float>(count);
      smooth(out_idx, 2)         = sum_z / static_cast<float>(count);
    }
  }

  return smooth;
}

// ======================================================================
// rotate_features 实现
// ======================================================================
rotate_features::rotate_features(const Eigen::VectorXf& angle) {
  const Eigen::Index B = angle.size();

  corrective_mat_2d_T_.resize(B, 4);  // 每行 4 个元素 = 2x2 矩阵行展开
  corrective_mat_Y_T_.resize(B, 9);   // 每行 9 个元素 = 3x3 矩阵行展开

  for (Eigen::Index i = 0; i < B; ++i) {
    const float cos_a          = std::cos(angle(i));
    const float sin_a          = std::sin(angle(i));

    // 2D 旋转矩阵转置: [[cos, sin], [-sin, cos]]
    corrective_mat_2d_T_(i, 0) = cos_a;
    corrective_mat_2d_T_(i, 1) = sin_a;
    corrective_mat_2d_T_(i, 2) = -sin_a;
    corrective_mat_2d_T_(i, 3) = cos_a;

    // 3D 绕 Y 轴旋转矩阵转置:
    // Ry^T = [[cos, 0, -sin], [0, 1, 0], [sin, 0, cos]]
    corrective_mat_Y_T_(i, 0)  = cos_a;
    corrective_mat_Y_T_(i, 1)  = 0.0f;
    corrective_mat_Y_T_(i, 2)  = -sin_a;
    corrective_mat_Y_T_(i, 3)  = 0.0f;
    corrective_mat_Y_T_(i, 4)  = 1.0f;
    corrective_mat_Y_T_(i, 5)  = 0.0f;
    corrective_mat_Y_T_(i, 6)  = sin_a;
    corrective_mat_Y_T_(i, 7)  = 0.0f;
    corrective_mat_Y_T_(i, 8)  = cos_a;
  }
}

MatrixXfRow rotate_features::rotate_positions(
    const MatrixXfRow& positions, std::int64_t batch_size, std::int64_t time_steps
) const {
  // positions: [B*T, 3]
  const Eigen::Index total = positions.rows();
  DOODLE_CHICK(total == batch_size * time_steps, "总帧数不匹配");
  DOODLE_CHICK(positions.cols() == 3, "列数 != 3");
  DOODLE_CHICK(corrective_mat_Y_T_.rows() == batch_size, "角度数不匹配 batch_size");

  MatrixXfRow result(total, 3);

  for (Eigen::Index b = 0; b < batch_size; ++b) {
    for (Eigen::Index t = 0; t < time_steps; ++t) {
      const Eigen::Index idx = b * time_steps + t;

      // positions @ Ry^T
      const float x          = positions(idx, 0);
      const float y          = positions(idx, 1);
      const float z          = positions(idx, 2);

      const float* R         = corrective_mat_Y_T_.row(b).data();
      result(idx, 0)         = x * R[0] + y * R[1] + z * R[2];
      result(idx, 1)         = x * R[3] + y * R[4] + z * R[5];
      result(idx, 2)         = x * R[6] + y * R[7] + z * R[8];
    }
  }

  return result;
}

MatrixXfRow rotate_features::rotate_2d_positions(
    const MatrixXfRow& positions_2d, std::int64_t batch_size, std::int64_t time_steps
) const {
  // positions_2d: [B*T, 2]
  const Eigen::Index total = positions_2d.rows();
  DOODLE_CHICK(total == batch_size * time_steps, "总帧数不匹配");
  DOODLE_CHICK(positions_2d.cols() == 2, "列数 != 2");

  MatrixXfRow result(total, 2);

  for (Eigen::Index b = 0; b < batch_size; ++b) {
    for (Eigen::Index t = 0; t < time_steps; ++t) {
      const Eigen::Index idx = b * time_steps + t;

      // positions @ R2d^T
      const float x          = positions_2d(idx, 0);
      const float y          = positions_2d(idx, 1);

      const float* R         = corrective_mat_2d_T_.row(b).data();
      result(idx, 0)         = x * R[0] + y * R[1];
      result(idx, 1)         = x * R[2] + y * R[3];
    }
  }

  return result;
}

MatrixXfRow rotate_features::rotate_6d_rotations(
    const MatrixXfRow& rotations_6d, std::int64_t batch_size, std::int64_t time_steps, std::int64_t nbjoints
) const {
  // rotations_6d: [B*T, J*6]
  const Eigen::Index total = rotations_6d.rows();
  DOODLE_CHICK(total == batch_size * time_steps, "总帧数不匹配");
  DOODLE_CHICK(rotations_6d.cols() == nbjoints * 6, "列数不匹配 J*6");

  // 先转成旋转矩阵 [B*T*J, 9], 旋转后再转回 6D
  const Eigen::Index total_joints = total * nbjoints;
  MatrixXfRow rot_mats(total_joints, 9);
  for (Eigen::Index i = 0; i < total_joints; ++i) {
    const Eigen::Index frame_idx = i / nbjoints;
    const Eigen::Index b         = frame_idx / time_steps;

    // 从 6D 恢复为矩阵
    const float* src_6d          = rotations_6d.row(i / nbjoints).data() + (i % nbjoints) * 6;
    Eigen::Vector3f x_raw(src_6d[0], src_6d[1], src_6d[2]);
    Eigen::Vector3f y_raw(src_6d[3], src_6d[4], src_6d[5]);

    Eigen::Vector3f x = x_raw.normalized();
    Eigen::Vector3f z = x.cross(y_raw).normalized();
    Eigen::Vector3f y = z.cross(x);

    // 应用旋转 Ry^T @ R
    const float* R    = corrective_mat_Y_T_.row(b).data();
    // R_rotated = Ry^T * R
    const float r00   = R[0] * x(0) + R[1] * x(1) + R[2] * x(2);
    const float r10   = R[3] * x(0) + R[4] * x(1) + R[5] * x(2);
    const float r20   = R[6] * x(0) + R[7] * x(1) + R[8] * x(2);
    const float r01   = R[0] * y(0) + R[1] * y(1) + R[2] * y(2);
    const float r11   = R[3] * y(0) + R[4] * y(1) + R[5] * y(2);
    const float r21   = R[6] * y(0) + R[7] * y(1) + R[8] * y(2);
    const float r02   = R[0] * z(0) + R[1] * z(1) + R[2] * z(2);
    const float r12   = R[3] * z(0) + R[4] * z(1) + R[5] * z(2);
    const float r22   = R[6] * z(0) + R[7] * z(1) + R[8] * z(2);

    rot_mats(i, 0)    = r00;
    rot_mats(i, 1)    = r01;
    rot_mats(i, 2)    = r02;
    rot_mats(i, 3)    = r10;
    rot_mats(i, 4)    = r11;
    rot_mats(i, 5)    = r12;
    rot_mats(i, 6)    = r20;
    rot_mats(i, 7)    = r21;
    rot_mats(i, 8)    = r22;
  }

  // 转回 6D
  MatrixXfRow result(total, nbjoints * 6);
  for (Eigen::Index i = 0; i < total_joints; ++i) {
    const Eigen::Index frame_idx     = i / nbjoints;
    const Eigen::Index j_idx         = i % nbjoints;

    result(frame_idx, j_idx * 6 + 0) = rot_mats(i, 0);
    result(frame_idx, j_idx * 6 + 1) = rot_mats(i, 3);
    result(frame_idx, j_idx * 6 + 2) = rot_mats(i, 6);
    result(frame_idx, j_idx * 6 + 3) = rot_mats(i, 1);
    result(frame_idx, j_idx * 6 + 4) = rot_mats(i, 4);
    result(frame_idx, j_idx * 6 + 5) = rot_mats(i, 7);
  }

  return result;
}

}  // namespace doodle::ai
