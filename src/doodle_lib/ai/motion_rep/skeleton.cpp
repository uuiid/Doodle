//
// Created by TD on 25-7-21.
//
#include "skeleton.h"

#include <doodle_core/exception/exception.h>

#include <queue>

namespace doodle::ai {

void skeleton::build_joint_levels() {
  joint_levels.clear();
  if (nbjoints <= 0) return;

  // 计算每个关节的深度
  std::vector<std::int64_t> depths(static_cast<std::size_t>(nbjoints), 0);
  std::int64_t max_depth = 0;

  for (std::int64_t i = 0; i < nbjoints; ++i) {
    std::int64_t depth = 0;
    std::int64_t j     = i;
    // 沿着父链回溯直到根
    while (joint_parents[static_cast<std::size_t>(j)] != -1) {
      j = joint_parents[static_cast<std::size_t>(j)];
      ++depth;
    }
    depths[static_cast<std::size_t>(i)] = depth;
    if (depth > max_depth) max_depth = depth;
  }

  // 按深度分组
  joint_levels.resize(static_cast<std::size_t>(max_depth + 1));
  for (std::int64_t i = 0; i < nbjoints; ++i) {
    joint_levels[static_cast<std::size_t>(depths[static_cast<std::size_t>(i)])].push_back(i);
  }
}

// ======================================================================
// 辅助：将 3x3 矩阵的行展开向量（9元素）转换为 4x4 变换矩阵
// ======================================================================
namespace {
Eigen::Matrix4f make_transform_mat(const float* rot_9, const float* trans_3) {
  Eigen::Matrix4f T = Eigen::Matrix4f::Identity();
  // 旋转部分
  T(0, 0) = rot_9[0];  T(0, 1) = rot_9[1];  T(0, 2) = rot_9[2];
  T(1, 0) = rot_9[3];  T(1, 1) = rot_9[4];  T(1, 2) = rot_9[5];
  T(2, 0) = rot_9[6];  T(2, 1) = rot_9[7];  T(2, 2) = rot_9[8];
  // 平移部分
  if (trans_3) {
    T(0, 3) = trans_3[0];
    T(1, 3) = trans_3[1];
    T(2, 3) = trans_3[2];
  }
  return T;
}

void matrix_9_to_4x4(const float* src_9, Eigen::Matrix4f& dst) {
  dst.setIdentity();
  dst(0, 0) = src_9[0]; dst(0, 1) = src_9[1]; dst(0, 2) = src_9[2];
  dst(1, 0) = src_9[3]; dst(1, 1) = src_9[4]; dst(1, 2) = src_9[5];
  dst(2, 0) = src_9[6]; dst(2, 1) = src_9[7]; dst(2, 2) = src_9[8];
}

void matrix_4x4_to_9(const Eigen::Matrix4f& src, float* dst_9) {
  dst_9[0] = src(0, 0); dst_9[1] = src(0, 1); dst_9[2] = src(0, 2);
  dst_9[3] = src(1, 0); dst_9[4] = src(1, 1); dst_9[5] = src(1, 2);
  dst_9[6] = src(2, 0); dst_9[7] = src(2, 1); dst_9[8] = src(2, 2);
}
}  // namespace

// ======================================================================
// FK 实现
// ======================================================================
fk_result fk(
    const Eigen::MatrixXf& local_rot_mats,
    const Eigen::MatrixXf& root_positions,
    const skeleton& skel
) {
  const Eigen::Index total_frames = local_rot_mats.rows();
  const Eigen::Index J            = skel.nbjoints;
  DOODLE_CHICK(local_rot_mats.cols() == J * 9, "local_rot_mats 列数 {} 不匹配 J*9 = {}", local_rot_mats.cols(), J * 9);
  DOODLE_CHICK(root_positions.rows() == total_frames, "root_positions 行数不匹配");
  DOODLE_CHICK(root_positions.cols() == 3, "root_positions 列数 != 3");

  fk_result result;
  result.global_rot_mats.resize(total_frames, J * 9);
  result.posed_joints.resize(total_frames, J * 3);
  result.posed_joints_norootpos.resize(total_frames, J * 3);

  // 构建 joint_levels（若尚未构建）
  if (skel.joint_levels.empty()) {
    auto skel_copy = skel;
    skel_copy.build_joint_levels();
    // 使用深拷贝后的 levels
    return fk(local_rot_mats, root_positions, skel_copy);
  }

  // 对每帧做 FK
  for (Eigen::Index f = 0; f < total_frames; ++f) {
    // 当前帧的旋转和位置
    const float* rot_row = local_rot_mats.row(f).data();
    const float* pos_row = root_positions.row(f).data();

    // 每关节的变换矩阵 [J, 4, 4]
    std::vector<Eigen::Matrix4f> transforms(static_cast<std::size_t>(J));

    // 第 0 层：根关节
    const auto root_i = static_cast<std::size_t>(skel.root_idx);
    {
      // 根关节的相对位置 = neutral_joints[root] 的偏移
      // 但 Python FK 中先减去了 pelvis_offset，所以这里相对位置为 0
      Eigen::Vector3f rel_pos(0, 0, 0);
      Eigen::Matrix4f local_T = Eigen::Matrix4f::Identity();
      matrix_9_to_4x4(rot_row + root_i * 9, local_T);
      local_T(0, 3) = rel_pos(0);
      local_T(1, 3) = rel_pos(1);
      local_T(2, 3) = rel_pos(2);

      // 根关节的全局变换 = local_T，然后应用根位置偏移
      transforms[root_i] = local_T;
      transforms[root_i](0, 3) += pos_row[0];
      transforms[root_i](1, 3) += pos_row[1];
      transforms[root_i](2, 3) += pos_row[2];
    }

    // 后续层级：逐级计算
    for (const auto& level : skel.joint_levels) {
      for (const auto& j_idx : level) {
        if (j_idx == skel.root_idx) continue;
        const auto ji   = static_cast<std::size_t>(j_idx);
        const auto pi   = static_cast<std::size_t>(skel.joint_parents[ji]);
        const auto& p_T = transforms[pi];

        // 相对位置 = neutral_joints[j] - neutral_joints[parent(j)]
        Eigen::Vector3f rel_joint;
        rel_joint(0) = skel.neutral_joints(j_idx, 0) - skel.neutral_joints(static_cast<Eigen::Index>(pi), 0);
        rel_joint(1) = skel.neutral_joints(j_idx, 1) - skel.neutral_joints(static_cast<Eigen::Index>(pi), 1);
        rel_joint(2) = skel.neutral_joints(j_idx, 2) - skel.neutral_joints(static_cast<Eigen::Index>(pi), 2);

        // 局部变换
        Eigen::Matrix4f local_T = Eigen::Matrix4f::Identity();
        matrix_9_to_4x4(rot_row + j_idx * 9, local_T);
        local_T(0, 3) = rel_joint(0);
        local_T(1, 3) = rel_joint(1);
        local_T(2, 3) = rel_joint(2);

        // 全局变换 = parent_global_T * local_T
        transforms[ji] = p_T * local_T;
      }
    }

    // 提取结果
    float* global_rot_out = result.global_rot_mats.row(f).data();
    float* posed_out      = result.posed_joints.row(f).data();
    float* posed_no_root  = result.posed_joints_norootpos.row(f).data();

    for (Eigen::Index j = 0; j < J; ++j) {
      const auto ji = static_cast<std::size_t>(j);
      // 全局旋转
      matrix_4x4_to_9(transforms[ji], global_rot_out + j * 9);

      // 全局位置
      posed_out[j * 3 + 0] = transforms[ji](0, 3);
      posed_out[j * 3 + 1] = transforms[ji](1, 3);
      posed_out[j * 3 + 2] = transforms[ji](2, 3);

      // 无根位置偏移 = 全局位置 - 根位置
      posed_no_root[j * 3 + 0] = transforms[ji](0, 3) - pos_row[0];
      posed_no_root[j * 3 + 1] = transforms[ji](1, 3) - pos_row[1];
      posed_no_root[j * 3 + 2] = transforms[ji](2, 3) - pos_row[2];
    }
  }

  return result;
}

// ======================================================================
// 全局旋转 → 局部旋转
// ======================================================================
Eigen::MatrixXf global_rots_to_local_rots(
    const Eigen::MatrixXf& global_rot_mats,
    const skeleton& skel
) {
  const Eigen::Index total = global_rot_mats.rows();
  const Eigen::Index J     = skel.nbjoints;
  DOODLE_CHICK(global_rot_mats.cols() == J * 9, "global_rot_mats 列数不匹配");

  Eigen::MatrixXf local_rot_mats(total, J * 9);

  for (Eigen::Index f = 0; f < total; ++f) {
    const float* global_row = global_rot_mats.row(f).data();
    float* local_row        = local_rot_mats.row(f).data();

    for (Eigen::Index j = 0; j < J; ++j) {
      const auto ji = static_cast<std::size_t>(j);

      // 全局旋转矩阵 R_global[j]
      Eigen::Matrix3f R_global;
      R_global(0, 0) = global_row[j * 9 + 0]; R_global(0, 1) = global_row[j * 9 + 1]; R_global(0, 2) = global_row[j * 9 + 2];
      R_global(1, 0) = global_row[j * 9 + 3]; R_global(1, 1) = global_row[j * 9 + 4]; R_global(1, 2) = global_row[j * 9 + 5];
      R_global(2, 0) = global_row[j * 9 + 6]; R_global(2, 1) = global_row[j * 9 + 7]; R_global(2, 2) = global_row[j * 9 + 8];

      Eigen::Matrix3f R_local;

      if (j == skel.root_idx) {
        // 根关节：局部 = 全局
        R_local = R_global;
      } else {
        // 非根关节：局部 = parent(R_global)^T * R_global[j]
        const auto pi = static_cast<std::size_t>(skel.joint_parents[ji]);

        Eigen::Matrix3f R_parent;
        R_parent(0, 0) = global_row[pi * 9 + 0]; R_parent(0, 1) = global_row[pi * 9 + 1]; R_parent(0, 2) = global_row[pi * 9 + 2];
        R_parent(1, 0) = global_row[pi * 9 + 3]; R_parent(1, 1) = global_row[pi * 9 + 4]; R_parent(1, 2) = global_row[pi * 9 + 5];
        R_parent(2, 0) = global_row[pi * 9 + 6]; R_parent(2, 1) = global_row[pi * 9 + 7]; R_parent(2, 2) = global_row[pi * 9 + 8];

        R_local = R_parent.transpose() * R_global;
      }

      // 写入输出
      local_row[j * 9 + 0] = R_local(0, 0); local_row[j * 9 + 1] = R_local(0, 1); local_row[j * 9 + 2] = R_local(0, 2);
      local_row[j * 9 + 3] = R_local(1, 0); local_row[j * 9 + 4] = R_local(1, 1); local_row[j * 9 + 5] = R_local(1, 2);
      local_row[j * 9 + 6] = R_local(2, 0); local_row[j * 9 + 7] = R_local(2, 1); local_row[j * 9 + 8] = R_local(2, 2);
    }
  }

  return local_rot_mats;
}

}  // namespace doodle::ai
