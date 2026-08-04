//
// Created by TD on 26-8-4.
//
#include "motion_postprocess.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/exception/exception.h>

#include <doodle_lib/ai/AnimProcessing/Utility.h>
#include <doodle_lib/ai/Math/Transform.h>
#include <doodle_lib/ai/motion_rep/geometry.h>

#include <algorithm>
#include <boost/numeric/conversion/cast.hpp>
#include <cmath>
#include <spdlog/spdlog.h>
#include <unordered_map>

namespace doodle::ai {

// ======================================================================
// create_working_rig_from_skeleton
// ======================================================================

std::vector<working_rig_joint> create_working_rig_from_skeleton(
    const skeleton_base& skeleton, float above_ground_offset
) {
  const auto& joint_names       = skeleton.bone_order_names_;
  const auto& neutral_positions = skeleton.neutral_joints_;  // [J, 3]
  const auto& parent_indices    = skeleton.joint_parents_;
  const auto J                  = skeleton.nbjoints_;

  // 计算最低点（脚趾）高度，用于离地偏移
  float toe_height              = neutral_positions.col(1).minCoeff();

  std::vector<working_rig_joint> rig;
  rig.reserve(J);

  for (std::int64_t i = 0; i < J; ++i) {
    working_rig_joint joint;
    joint.name            = joint_names[i];
    joint.parent          = (parent_indices[i] == -1) ? "" : joint_names[parent_indices[i]];
    joint.t_pose_rotation = {0.0f, 0.0f, 0.0f, 1.0f};  // 单位四元数 (x, y, z, w)，匹配 Math::Quaternion 构造

    if (parent_indices[i] == -1) {
      // 根关节：移动使最低点位于地面以上
      joint.t_pose_translation = {
          neutral_positions(i, 0), neutral_positions(i, 1) - toe_height + above_ground_offset, neutral_positions(i, 2)
      };
    } else {
      const auto parent_idx    = parent_indices[i];
      joint.t_pose_translation = {
          neutral_positions(i, 0) - neutral_positions(parent_idx, 0),
          neutral_positions(i, 1) - neutral_positions(parent_idx, 1),
          neutral_positions(i, 2) - neutral_positions(parent_idx, 2)
      };
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
    return a->type_name() == fullbody_constraint_set::name   ? false
           : b->type_name() == fullbody_constraint_set::name ? true
                                                             : false;
  });

  for (const auto& constraint : sorted) {
    // 获取有效帧索引（过滤超出范围的索引）
    const auto& raw_indices = constraint->get_frame_indices();

    std::vector<std::int64_t> valid_indices;
    std::vector<std::int64_t> valid_positions;  // 在原始数组中的位置
    for (Eigen::Index k = 0; k < raw_indices.size(); ++k) {
      if (raw_indices(k) < num_frames) {
        valid_indices.push_back(raw_indices(k));
        valid_positions.push_back(k);
      }
    }
    if (valid_indices.empty()) continue;

    const auto K = boost::numeric_cast<Eigen::Index>(valid_indices.size());

    // Root2DConstraintSet: 仅设置 xz
    if (auto* r = dynamic_cast<const root2d_constraint_set*>(constraint.get())) {
      for (Eigen::Index k = 0; k < K; ++k) {
        auto f                 = valid_indices[k];
        auto p                 = valid_positions[k];
        hip_translations(f, 0) = r->smooth_root_2d_(p, 0);  // x
        hip_translations(f, 2) = r->smooth_root_2d_(p, 1);  // z
      }
      continue;
    }

    // FullBody / EndEffector
    const auto* global_rots = [&]() -> const MatrixXfRow* {
      if (auto* f = dynamic_cast<const fullbody_constraint_set*>(constraint.get())) return &f->global_joints_rots_;
      if (auto* e = dynamic_cast<const end_effector_constraint_set*>(constraint.get())) return &e->global_joints_rots_;
      return nullptr;
    }();
    const auto* global_positions = [&]() -> const MatrixXfRow* {
      if (auto* f = dynamic_cast<const fullbody_constraint_set*>(constraint.get())) return &f->global_joints_positions_;
      if (auto* e = dynamic_cast<const end_effector_constraint_set*>(constraint.get()))
        return &e->global_joints_positions_;
      return nullptr;
    }();
    const auto* smooth_root_2d = [&]() -> const MatrixXfRow* {
      if (auto* f = dynamic_cast<const fullbody_constraint_set*>(constraint.get())) return &f->smooth_root_2d_;
      if (auto* e = dynamic_cast<const end_effector_constraint_set*>(constraint.get())) return &e->smooth_root_2d_;
      return nullptr;
    }();

    if (!global_rots || !global_positions || !smooth_root_2d) continue;

    // 提取有效帧的全局旋转和位置
    MatrixXfRow valid_global_rots(K, num_joints * 9);
    MatrixXfRow valid_global_positions(K, num_joints * 3);
    MatrixXfRow valid_smooth_root_2d(K, 2);
    for (Eigen::Index k = 0; k < K; ++k) {
      auto p                        = valid_positions[k];
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
      auto f                  = valid_indices[k];
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
      : full_body(num_frames, 0.0f),
        left_foot(num_frames, 0.0f),
        right_foot(num_frames, 0.0f),
        left_hand(num_frames, 0.0f),
        right_hand(num_frames, 0.0f),
        root(num_frames, 0.0f) {}
};

static constraint_masks build_constraint_masks(
    const std::vector<constraint_set_ptr>& constraints, std::int64_t num_frames
) {
  constraint_masks masks(num_frames);

  for (const auto& c : constraints) {
    // 获取帧索引
    const auto& frame_indices  = c->get_frame_indices();

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

    for (Eigen::Index k = 0; k < frame_indices.size(); ++k) {
      auto idx = frame_indices(k);
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
    const std::vector<constraint_set_ptr>& constraint_lst, float contact_threshold, float root_margin
) {
  const auto num_joints  = skeleton.nbjoints_;
  const auto T           = num_frames;
  const auto B           = batch_size;

  // 构建约束掩码（所有批次共享）
  auto shared_masks    = build_constraint_masks(constraint_lst, T);
  std::vector<constraint_masks> masks_per_batch;
  for (std::int64_t b = 0; b < B; ++b) {
    masks_per_batch.push_back(shared_masks);
  }

  // 创建工作骨骼
  const bool is_soma =
      skeleton.name_.find("soma") != std::string::npos || skeleton.name_.find("SOMA") != std::string::npos;
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

  if (!constraint_lst.empty()) {
    for (std::int64_t b = 0; b < B; ++b) {
      auto [hip_t, rots_t] = extract_input_motion_from_constraints(constraint_lst, skeleton, T, num_joints);
      // hip_t: [T, 3], rots_t: [T, J*4]
      hip_input.block(b * T, 0, T, 3)               = hip_t;
      rots_input.block(b * T, 0, T, num_joints * 4) = rots_t;
    }
  }

  // 确定是否有双踝关节
  bool has_double_ankle_joints = false;
  {
    // G1Skeleton34 有双踝关节；通过骨骼名称判断
    const auto& name        = skeleton.name_;
    has_double_ankle_joints = name.find("G1") != std::string::npos || name.find("g1") != std::string::npos;
  }

  // ======================================================================
  // 构建 Math 骨骼数据（与 Python BindingsPython.cpp 一致）
  // ======================================================================

  // defaultPose: 从 working_rig 构建
  std::vector<Math::Transform> defaultPose(num_joints);
  for (std::int64_t j = 0; j < num_joints; ++j) {
    const auto& rj = working_rig[j];
    defaultPose[j].SetTranslation(
        Math::Vector(rj.t_pose_translation[0], rj.t_pose_translation[1], rj.t_pose_translation[2])
    );
    // t_pose_rotation: (x, y, z, w) → Math::Quaternion(x, y, z, w)
    defaultPose[j].SetRotation(
        Math::Quaternion(rj.t_pose_rotation[0], rj.t_pose_rotation[1], rj.t_pose_rotation[2], rj.t_pose_rotation[3])
    );
  }

  // joint_parents_vec: 名称→索引 映射
  std::unordered_map<std::string, int> name_to_idx;
  for (int j = 0; j < boost::numeric_cast<int>(num_joints); ++j) name_to_idx[working_rig[j].name] = j;

  std::vector<int> joint_parents_vec(num_joints);
  for (int j = 0; j < boost::numeric_cast<int>(num_joints); ++j) {
    joint_parents_vec[j] = working_rig[j].parent.empty() ? -1 : name_to_idx.at(working_rig[j].parent);
  }

  // 从 skeleton 语义索引获取手/脚关节
  DOODLE_CHICK(
      !skeleton.left_hand_joint_idx_.empty() && !skeleton.right_hand_joint_idx_.empty() &&
          !skeleton.left_foot_joint_idx_.empty() && !skeleton.right_foot_joint_idx_.empty(),
      "Skeleton must have hand and foot joints defined"
  );
  int left_hand_idx  = boost::numeric_cast<int>(skeleton.left_hand_joint_idx_[0]);
  int right_hand_idx = boost::numeric_cast<int>(skeleton.right_hand_joint_idx_[0]);
  int left_foot_idx  = boost::numeric_cast<int>(skeleton.left_foot_joint_idx_[0]);
  int right_foot_idx = boost::numeric_cast<int>(skeleton.right_foot_joint_idx_[0]);

  // 查找脚趾关节（父关节为脚）
  int left_toe_idx = -1, right_toe_idx = -1;
  for (int j = 0; j < boost::numeric_cast<int>(num_joints); ++j) {
    if (joint_parents_vec[j] == left_foot_idx) left_toe_idx = j;
    if (joint_parents_vec[j] == right_foot_idx) right_toe_idx = j;
  }

  // ======================================================================
  // 逐批次调用 Animation::CorrectMotion
  // ======================================================================

  for (std::int64_t b = 0; b < B; ++b) {
    const auto& masks = masks_per_batch[b];

    // --- posesFixed: 待校正的当前运动 [T][J] ---
    std::vector<std::vector<Math::Transform>> posesFixed(T, defaultPose);
    for (int f = 0; f < boost::numeric_cast<int>(T); ++f) {
      auto bt = boost::numeric_cast<Eigen::Index>(b * T + f);
      posesFixed[f][0].SetTranslation(Math::Vector(hip_corrected(bt, 0), hip_corrected(bt, 1), hip_corrected(bt, 2)));
      for (int j = 0; j < boost::numeric_cast<int>(num_joints); ++j) {
        // rots_corrected: (w, x, y, z) → Math::Quaternion(x, y, z, w)
        Math::Quaternion q(
            rots_corrected(bt, j * 4 + 1), rots_corrected(bt, j * 4 + 2), rots_corrected(bt, j * 4 + 3),
            rots_corrected(bt, j * 4 + 0)
        );
        q.Normalize();
        posesFixed[f][j].SetRotation(q);
      }
    }

    // --- posesTarget: 目标运动（来自约束）[T][J] ---
    std::vector<std::vector<Math::Transform>> posesTarget(T, defaultPose);
    for (int f = 0; f < boost::numeric_cast<int>(T); ++f) {
      auto bt = boost::numeric_cast<Eigen::Index>(b * T + f);
      posesTarget[f][0].SetTranslation(Math::Vector(hip_input(bt, 0), hip_input(bt, 1), hip_input(bt, 2)));
      for (int j = 0; j < boost::numeric_cast<int>(num_joints); ++j) {
        // rots_input: (w, x, y, z) → Math::Quaternion(x, y, z, w)
        Math::Quaternion q(
            rots_input(bt, j * 4 + 1), rots_input(bt, j * 4 + 2), rots_input(bt, j * 4 + 3), rots_input(bt, j * 4 + 0)
        );
        q.Normalize();
        posesTarget[f][j].SetRotation(q);
      }
    }

    // --- endEffectorPins: 左右手 + 左右脚 ---
    std::vector<Animation::ContactInfo> endEffectorPins(4);
    endEffectorPins[0].jointIndex = left_hand_idx;
    endEffectorPins[0].hintOffset = Math::Vector(0.0f, 0.0f, -0.1f);
    endEffectorPins[1].jointIndex = right_hand_idx;
    endEffectorPins[1].hintOffset = Math::Vector(0.0f, 0.0f, -0.1f);
    endEffectorPins[2].jointIndex = left_foot_idx;
    endEffectorPins[2].hintOffset = Math::Vector(0.0f, 0.0f, 0.1f);
    endEffectorPins[3].jointIndex = right_foot_idx;
    endEffectorPins[3].hintOffset = Math::Vector(0.0f, 0.0f, 0.1f);

    for (int f = 0; f < boost::numeric_cast<int>(T); ++f) {
      endEffectorPins[0].contactMask.push_back((1.0f - masks.full_body[f]) * masks.left_hand[f]);
      endEffectorPins[1].contactMask.push_back((1.0f - masks.full_body[f]) * masks.right_hand[f]);
      endEffectorPins[2].contactMask.push_back((1.0f - masks.full_body[f]) * masks.left_foot[f]);
      endEffectorPins[3].contactMask.push_back((1.0f - masks.full_body[f]) * masks.right_foot[f]);
    }

    // --- contactInfo: 脚接触（2-bone IK）+ 可选脚趾（1-bone IK）---
    std::vector<Animation::ContactInfo> contactInfo(2);

    auto footTrans = Animation::JointLocalToGlobal(joint_parents_vec, right_foot_idx, defaultPose).GetTranslation();
    contactInfo[0].jointIndex = right_foot_idx;
    contactInfo[0].hintOffset = Math::Vector(0.0f, 0.0f, 0.1f);
    contactInfo[0].minHeight  = footTrans.GetY();

    footTrans = Animation::JointLocalToGlobal(joint_parents_vec, left_foot_idx, defaultPose).GetTranslation();
    contactInfo[1].jointIndex = left_foot_idx;
    contactInfo[1].hintOffset = Math::Vector(0.0f, 0.0f, 0.1f);
    contactInfo[1].minHeight  = footTrans.GetY();

    auto& rContacts           = contactInfo[0].contactMask;
    auto& lContacts           = contactInfo[1].contactMask;
    rContacts.resize(T);
    lContacts.resize(T);
    for (int f = 0; f < boost::numeric_cast<int>(T); ++f) {
      auto bt      = boost::numeric_cast<Eigen::Index>(b * T + f);
      // contacts 布局: [left_heel, left_toe, right_heel, right_toe]
      rContacts[f] = masks.right_foot[f] ? 0.0f : contacts(bt, 2);
      lContacts[f] = masks.left_foot[f] ? 0.0f : contacts(bt, 0);
      // 合并脚趾接触
      rContacts[f] = std::min((masks.right_foot[f] ? 0.0f : contacts(bt, 3)) + rContacts[f], 1.0f);
      lContacts[f] = std::min((masks.left_foot[f] ? 0.0f : contacts(bt, 1)) + lContacts[f], 1.0f);
    }

    if (left_toe_idx != -1 && right_toe_idx != -1) {
      auto toeTrans = Animation::JointLocalToGlobal(joint_parents_vec, right_toe_idx, defaultPose).GetTranslation();

      contactInfo.resize(4);
      contactInfo[2].jointIndex  = right_toe_idx;
      contactInfo[2].contactType = Animation::kOneBone;
      contactInfo[2].minHeight   = toeTrans.GetY();
      contactInfo[3].jointIndex  = left_toe_idx;
      contactInfo[3].contactType = Animation::kOneBone;
      contactInfo[3].minHeight   = toeTrans.GetY();

      auto& rToeContacts         = contactInfo[2].contactMask;
      auto& lToeContacts         = contactInfo[3].contactMask;
      rToeContacts.resize(T);
      lToeContacts.resize(T);
      for (int f = 0; f < boost::numeric_cast<int>(T); ++f) {
        auto bt         = boost::numeric_cast<Eigen::Index>(b * T + f);
        rToeContacts[f] = masks.right_foot[f] ? 0.0f : contacts(bt, 3);
        lToeContacts[f] = masks.left_foot[f] ? 0.0f : contacts(bt, 1);
      }
    }

    // --- 调用核心校正 ---
    Animation::CorrectMotion(
        posesFixed, posesTarget, masks.full_body, masks.root, contactInfo, endEffectorPins, joint_parents_vec,
        defaultPose, contact_threshold, root_margin, has_double_ankle_joints
    );

    // --- 写回结果 ---
    for (int f = 0; f < boost::numeric_cast<int>(T); ++f) {
      auto bt              = boost::numeric_cast<Eigen::Index>(b * T + f);
      auto t               = posesFixed[f][0].GetTranslation();
      hip_corrected(bt, 0) = t.GetX();
      hip_corrected(bt, 1) = t.GetY();
      hip_corrected(bt, 2) = t.GetZ();

      for (int j = 0; j < boost::numeric_cast<int>(num_joints); ++j) {
        auto q                        = posesFixed[f][j].GetRotation();
        // Math::Quaternion 内部: ((float*)&q)[0]=x, [1]=y, [2]=z, [3]=w
        // rots_corrected 存储: (w, x, y, z)
        rots_corrected(bt, j * 4 + 0) = reinterpret_cast<const float*>(&q)[3];  // w
        rots_corrected(bt, j * 4 + 1) = reinterpret_cast<const float*>(&q)[0];  // x
        rots_corrected(bt, j * 4 + 2) = reinterpret_cast<const float*>(&q)[1];  // y
        rots_corrected(bt, j * 4 + 3) = reinterpret_cast<const float*>(&q)[2];  // z
      }
    }
  }

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