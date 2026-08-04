//
// Created by TD on 26-8-4.
//
#include "motion_postprocess.h"

#include <doodle_lib/ai/motion_rep/geometry.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace doodle::ai {

// ======================================================================
// create_working_rig_from_skeleton
// ======================================================================

std::vector<working_rig_joint> create_working_rig_from_skeleton(
    const skeleton_base& skeleton, float above_ground_offset
) {
  const auto& joint_names      = skeleton.bone_order_names_;
  const auto& neutral_positions = skeleton.neutral_joints_;  // [J, 3]
  const auto& parent_indices   = skeleton.joint_parents_;
  const auto J                 = skeleton.nbjoints_;

  // 构建重定向映射
  std::unordered_map<std::string, std::string> retarget_map;
  {
    // 判断骨骼类型：SOMA 使用名称映射，G1/SMPLX 使用索引映射
    const bool is_soma = skeleton.name_.find("soma") != std::string::npos ||
                         skeleton.name_.find("SOMA") != std::string::npos;

    if (is_soma) {
      // SOMA: 名称到自身的映射
      for (const auto& tag : {"Hips", "Head", "LeftHand", "RightHand", "LeftFoot", "RightFoot"}) {
        retarget_map[tag] = tag;
      }
    } else {
      // G1/SMPLX: 使用语义索引
      if (!skeleton.left_hand_joint_idx_.empty())
        retarget_map[joint_names[skeleton.left_hand_joint_idx_[0]]] = "LeftHand";
      if (!skeleton.right_hand_joint_idx_.empty())
        retarget_map[joint_names[skeleton.right_hand_joint_idx_[0]]] = "RightHand";
      if (!skeleton.left_foot_joint_idx_.empty())
        retarget_map[joint_names[skeleton.left_foot_joint_idx_[0]]] = "LeftFoot";
      if (!skeleton.right_foot_joint_idx_.empty())
        retarget_map[joint_names[skeleton.right_foot_joint_idx_[0]]] = "RightFoot";
      retarget_map[joint_names[skeleton.root_idx_]] = "Hips";
    }
  }

  // 计算最低点（脚趾）高度，用于离地偏移
  float toe_height = neutral_positions.col(1).minCoeff();

  std::vector<working_rig_joint> rig;
  rig.reserve(J);

  for (std::int64_t i = 0; i < J; ++i) {
    working_rig_joint joint;
    joint.name              = joint_names[i];
    joint.parent            = (parent_indices[i] == -1) ? "" : joint_names[parent_indices[i]];
    joint.t_pose_rotation   = {1.0f, 0.0f, 0.0f, 0.0f};  // 单位四元数 (w, x, y, z)

    if (parent_indices[i] == -1) {
      // 根关节：移动使最低点位于地面以上
      joint.t_pose_translation = {
          neutral_positions(i, 0),
          neutral_positions(i, 1) - toe_height + above_ground_offset,
          neutral_positions(i, 2)};
    } else {
      const auto parent_idx = parent_indices[i];
      joint.t_pose_translation = {
          neutral_positions(i, 0) - neutral_positions(parent_idx, 0),
          neutral_positions(i, 1) - neutral_positions(parent_idx, 1),
          neutral_positions(i, 2) - neutral_positions(parent_idx, 2)};
    }

    // 设置重定向标签
    auto it = retarget_map.find(joint.name);
    if (it != retarget_map.end()) {
      joint.retarget_tag = it->second;
    }

    rig.push_back(std::move(joint));
  }

  return rig;
}

// ======================================================================
// extract_input_motion_from_constraints
// ======================================================================

std::pair<MatrixXfRow, MatrixXfRow> extract_input_motion_from_constraints(
    const std::vector<constraint_set_ptr>& constraint_lst, const skeleton_base& skeleton, std::int64_t num_frames,
    std::int64_t num_joints
) {
  // 初始化：髋部平移为 0，旋转为单位四元数
  MatrixXfRow hip_translations = MatrixXfRow::Zero(num_frames, 3);
  MatrixXfRow rotations        = MatrixXfRow::Zero(num_frames, num_joints * 4);
  // 单位四元数 (w=1, x=y=z=0)
  for (std::int64_t f = 0; f < num_frames; ++f) {
    for (std::int64_t j = 0; j < num_joints; ++j) {
      rotations(f, j * 4 + 0) = 1.0f;  // w
    }
  }

  if (constraint_lst.empty()) {
    return {hip_translations, rotations};
  }

  // 排序：FullBody 放最后处理，确保它覆盖其他约束
  auto sorted = constraint_lst;
  std::stable_sort(sorted.begin(), sorted.end(), [](const constraint_set_ptr& a, const constraint_set_ptr& b) {
    return a->type_name() == fullbody_constraint_set::name ? false
         : b->type_name() == fullbody_constraint_set::name ? true
                                                           : false;
  });

  for (const auto& constraint : sorted) {
    // 获取有效帧索引（过滤超出范围的索引）
    const auto& raw_indices = [&]() -> const Eigen::VectorXi& {
      if (auto* r = dynamic_cast<const root2d_constraint_set*>(constraint.get()))
        return r->frame_indices_;
      if (auto* f = dynamic_cast<const fullbody_constraint_set*>(constraint.get()))
        return f->frame_indices_;
      if (auto* e = dynamic_cast<const end_effector_constraint_set*>(constraint.get()))
        return e->frame_indices_;
      throw std::runtime_error("Unknown constraint type");
    }();

    std::vector<std::int64_t> valid_indices;
    std::vector<std::int64_t> valid_positions;  // 在原始数组中的位置
    for (Eigen::Index k = 0; k < raw_indices.size(); ++k) {
      if (raw_indices(k) < num_frames) {
        valid_indices.push_back(raw_indices(k));
        valid_positions.push_back(k);
      }
    }
    if (valid_indices.empty()) continue;

    const auto K = static_cast<Eigen::Index>(valid_indices.size());

    // Root2DConstraintSet: 仅设置 xz
    if (auto* r = dynamic_cast<const root2d_constraint_set*>(constraint.get())) {
      for (Eigen::Index k = 0; k < K; ++k) {
        auto f                        = valid_indices[k];
        auto p                        = valid_positions[k];
        hip_translations(f, 0)        = r->smooth_root_2d_(p, 0);  // x
        hip_translations(f, 2)        = r->smooth_root_2d_(p, 1);  // z
      }
      continue;
    }

    // FullBody / EndEffector
    const auto* global_rots      = [&]() -> const MatrixXfRow* {
      if (auto* f = dynamic_cast<const fullbody_constraint_set*>(constraint.get()))
        return &f->global_joints_rots_;
      if (auto* e = dynamic_cast<const end_effector_constraint_set*>(constraint.get()))
        return &e->global_joints_rots_;
      return nullptr;
    }();
    const auto* global_positions = [&]() -> const MatrixXfRow* {
      if (auto* f = dynamic_cast<const fullbody_constraint_set*>(constraint.get()))
        return &f->global_joints_positions_;
      if (auto* e = dynamic_cast<const end_effector_constraint_set*>(constraint.get()))
        return &e->global_joints_positions_;
      return nullptr;
    }();
    const auto* smooth_root_2d   = [&]() -> const MatrixXfRow* {
      if (auto* f = dynamic_cast<const fullbody_constraint_set*>(constraint.get()))
        return &f->smooth_root_2d_;
      if (auto* e = dynamic_cast<const end_effector_constraint_set*>(constraint.get()))
        return &e->smooth_root_2d_;
      return nullptr;
    }();

    if (!global_rots || !global_positions || !smooth_root_2d) continue;

    // 提取有效帧的全局旋转和位置
    MatrixXfRow valid_global_rots(K, num_joints * 9);
    MatrixXfRow valid_global_positions(K, num_joints * 3);
    MatrixXfRow valid_smooth_root_2d(K, 2);
    for (Eigen::Index k = 0; k < K; ++k) {
      auto p = valid_positions[k];
      valid_global_rots.row(k)      = global_rots->row(p);
      valid_global_positions.row(k) = global_positions->row(p);
      valid_smooth_root_2d.row(k)   = smooth_root_2d->row(p);
    }

    // 根位置
    MatrixXfRow root_positions(K, 3);
    for (Eigen::Index k = 0; k < K; ++k) {
      root_positions(k, 0) = valid_global_positions(k, skeleton.root_idx_ * 3 + 0);
      root_positions(k, 1) = valid_global_positions(k, skeleton.root_idx_ * 3 + 1);
      root_positions(k, 2) = valid_global_positions(k, skeleton.root_idx_ * 3 + 2);
    }

    // 对于 EE 约束（不包含 Hips），用 smooth_root_2d 替换 xz
    if (auto* e = dynamic_cast<const end_effector_constraint_set*>(constraint.get())) {
      bool has_hips = false;
      for (const auto& name : e->joint_names_) {
        if (name == "Hips") {
          has_hips = true;
          break;
        }
      }
      if (!has_hips) {
        root_positions.col(0) = valid_smooth_root_2d.col(0);  // x
        root_positions.col(2) = valid_smooth_root_2d.col(1);  // z
      }
    }

    // 全局旋转 → 局部旋转 → 四元数
    MatrixXfRow local_rot_mats = skeleton.global_rots_to_local_rots(valid_global_rots);  // [K, J*9]
    // 重塑为 [K*J, 9] 以应用 matrix_to_quaternion
    MatrixXfRow local_rot_flat(K * num_joints, 9);
    for (Eigen::Index k = 0; k < K; ++k) {
      for (Eigen::Index j = 0; j < num_joints; ++j) {
        local_rot_flat.row(k * num_joints + j) = local_rot_mats.block(k, j * 9, 1, 9);
      }
    }
    MatrixXfRow local_rot_quats_flat = matrix_to_quaternion(local_rot_flat);  // [K*J, 4]

    // 写回结果
    for (Eigen::Index k = 0; k < K; ++k) {
      auto f = valid_indices[k];
      hip_translations.row(f) = root_positions.row(k);
      for (Eigen::Index j = 0; j < num_joints; ++j) {
        rotations.block(f, j * 4, 1, 4) = local_rot_quats_flat.block(k * num_joints + j, 0, 1, 4);
      }
    }
  }

  return {hip_translations, rotations};
}

// ======================================================================
// 辅助：从约束列表构建掩码字典
// ======================================================================

struct constraint_masks {
  std::vector<float> full_body;   // [T]
  std::vector<float> left_foot;   // [T]
  std::vector<float> right_foot;  // [T]
  std::vector<float> left_hand;   // [T]
  std::vector<float> right_hand;  // [T]
  std::vector<float> root;        // [T]

  explicit constraint_masks(std::int64_t num_frames)
      : full_body(num_frames, 0.0f), left_foot(num_frames, 0.0f), right_foot(num_frames, 0.0f),
        left_hand(num_frames, 0.0f), right_hand(num_frames, 0.0f), root(num_frames, 0.0f) {}
};

static constraint_masks build_constraint_masks(
    const std::vector<constraint_set_ptr>& constraints, std::int64_t num_frames
) {
  constraint_masks masks(num_frames);

  for (const auto& c : constraints) {
    // 获取帧索引
    const Eigen::VectorXi* frame_indices = nullptr;
    if (auto* r = dynamic_cast<const root2d_constraint_set*>(c.get()))
      frame_indices = &r->frame_indices_;
    else if (auto* f = dynamic_cast<const fullbody_constraint_set*>(c.get()))
      frame_indices = &f->frame_indices_;
    else if (auto* e = dynamic_cast<const end_effector_constraint_set*>(c.get()))
      frame_indices = &e->frame_indices_;
    if (!frame_indices) continue;

    // 选择目标掩码
    std::vector<float>* target = nullptr;
    const auto& type           = c->type_name();
    if (type == fullbody_constraint_set::name)
      target = &masks.full_body;
    else if (type == left_foot_constraint_set::name)
      target = &masks.left_foot;
    else if (type == right_foot_constraint_set::name)
      target = &masks.right_foot;
    else if (type == left_hand_constraint_set::name)
      target = &masks.left_hand;
    else if (type == right_hand_constraint_set::name)
      target = &masks.right_hand;
    else if (type == root2d_constraint_set::name)
      target = &masks.root;
    if (!target) continue;

    for (Eigen::Index k = 0; k < frame_indices->size(); ++k) {
      auto idx = (*frame_indices)(k);
      if (idx < num_frames) (*target)[idx] = 1.0f;
    }
  }

  return masks;
}

// ======================================================================
// post_process_motion
// ======================================================================

post_process_result post_process_motion(
    const MatrixXfRow& local_rot_mats, const MatrixXfRow& root_positions, const MatrixXfRow& contacts,
    const skeleton_base& skeleton, std::int64_t batch_size, std::int64_t num_frames,
    const std::vector<constraint_set_ptr>& constraint_lst,
    const std::vector<std::vector<constraint_set_ptr>>& batched_constraints, float contact_threshold, float root_margin
) {
  const auto num_joints = skeleton.nbjoints_;
  const auto T          = num_frames;
  const auto B          = batch_size;

  // 确定是否使用批次约束
  const bool use_batched = !batched_constraints.empty();

  // 构建约束掩码
  std::vector<constraint_masks> masks_per_batch;
  if (use_batched) {
    masks_per_batch.reserve(B);
    for (std::int64_t b = 0; b < B; ++b) {
      masks_per_batch.emplace_back(build_constraint_masks(batched_constraints[b], T));
    }
  } else {
    auto shared_masks = build_constraint_masks(constraint_lst, T);
    for (std::int64_t b = 0; b < B; ++b) {
      masks_per_batch.push_back(shared_masks);
    }
  }

  // 创建工作骨骼
  const bool is_soma = skeleton.name_.find("soma") != std::string::npos ||
                       skeleton.name_.find("SOMA") != std::string::npos;
  float above_ground_offset = is_soma ? 0.02f : 0.007f;
  auto working_rig          = create_working_rig_from_skeleton(skeleton, above_ground_offset);

  // 克隆并准备数据
  MatrixXfRow hip_corrected = root_positions;  // [B*T, 3]
  // 将旋转矩阵转换为四元数 [B*T, J*4]
  MatrixXfRow rots_corrected(B * T, num_joints * 4);
  {
    // 重塑 [B*T, J*9] → [B*T*J, 9]
    MatrixXfRow flat_rots(B * T * num_joints, 9);
    for (Eigen::Index bt = 0; bt < B * T; ++bt) {
      for (Eigen::Index j = 0; j < num_joints; ++j) {
        flat_rots.row(bt * num_joints + j) = local_rot_mats.block(bt, j * 9, 1, 9);
      }
    }
    MatrixXfRow flat_quats = matrix_to_quaternion(flat_rots);  // [B*T*J, 4]
    for (Eigen::Index bt = 0; bt < B * T; ++bt) {
      for (Eigen::Index j = 0; j < num_joints; ++j) {
        rots_corrected.block(bt, j * 4, 1, 4) = flat_quats.block(bt * num_joints + j, 0, 1, 4);
      }
    }
  }

  // 提取输入运动
  MatrixXfRow hip_input(B * T, 3);
  hip_input.setZero();
  MatrixXfRow rots_input(B * T, num_joints * 4);
  rots_input.setZero();
  // 初始化为单位四元数
  for (Eigen::Index bt = 0; bt < B * T; ++bt) {
    for (Eigen::Index j = 0; j < num_joints; ++j) {
      rots_input(bt, j * 4 + 0) = 1.0f;
    }
  }

  if (!constraint_lst.empty() || use_batched) {
    for (std::int64_t b = 0; b < B; ++b) {
      const auto& constraints_for_batch =
          use_batched ? batched_constraints[b] : constraint_lst;

      auto [hip_t, rots_t] = extract_input_motion_from_constraints(
          constraints_for_batch, skeleton, T, num_joints
      );
      // hip_t: [T, 3], rots_t: [T, J*4]
      hip_input.block(b * T, 0, T, 3)                 = hip_t;
      rots_input.block(b * T, 0, T, num_joints * 4)   = rots_t;
    }
  }

  // 确定是否有双踝关节
  bool has_double_ankle_joints = false;
  {
    // G1Skeleton34 有双踝关节；通过骨骼名称判断
    const auto& name = skeleton.name_;
    has_double_ankle_joints =
        name.find("G1") != std::string::npos || name.find("g1") != std::string::npos;
  }

  // ======================================================================
  // correct_motion 占位
  // 对应 Python 中的 motion_correction.correct_motion() 调用
  // 在 C++ 中暂不实现，仅记录日志
  // ======================================================================
  spdlog::warn(
      "post_process_motion: correct_motion is not implemented in C++. "
      "Batch={}, Frames={}, Joints={}, contact_threshold={}, root_margin={}",
      B, T, num_joints, contact_threshold, root_margin
  );

  // 将四元数转换回旋转矩阵 [B*T, J*9]
  MatrixXfRow local_rot_mats_corrected(B * T, num_joints * 9);
  {
    // 重塑 [B*T, J*4] → [B*T*J, 4]
    MatrixXfRow flat_quats(B * T * num_joints, 4);
    for (Eigen::Index bt = 0; bt < B * T; ++bt) {
      for (Eigen::Index j = 0; j < num_joints; ++j) {
        flat_quats.row(bt * num_joints + j) = rots_corrected.block(bt, j * 4, 1, 4);
      }
    }
    MatrixXfRow flat_mats = quaternion_to_matrix(flat_quats);  // [B*T*J, 9]
    for (Eigen::Index bt = 0; bt < B * T; ++bt) {
      for (Eigen::Index j = 0; j < num_joints; ++j) {
        local_rot_mats_corrected.block(bt, j * 9, 1, 9) = flat_mats.block(bt * num_joints + j, 0, 1, 9);
      }
    }
  }

  // 前向运动学
  auto fk_res = skeleton.fk(local_rot_mats_corrected, hip_corrected);

  return {
      std::move(local_rot_mats_corrected),
      std::move(hip_corrected),
      std::move(fk_res.posed_joints),
      std::move(fk_res.global_rot_mats),
  };
}

}  // namespace doodle::ai