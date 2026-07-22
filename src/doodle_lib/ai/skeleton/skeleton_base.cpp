//
// Created by TD on 25-7-22.
//
#include "skeleton_base.h"

#include <doodle_core/exception/exception.h>

#include <cnpy.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace doodle::ai {

// ======================================================================
// 辅助：从 npy 文件加载矩阵
// ======================================================================
namespace {
Eigen::MatrixXf load_npy_matrix(const FSys::path& file_path, std::int64_t expected_cols = -1) {
  DOODLE_CHICK(FSys::exists(file_path), "npy 文件不存在: {}", file_path.string());

  auto data = cnpy::npy_load(file_path.string());
  DOODLE_CHICK(data.shape.size() >= 1, "npy 文件 {} shape 无效", file_path.string());

  // 计算总元素数
  std::size_t total = 1;
  for (auto& s : data.shape) total *= s;

  std::int64_t rows = 1;
  std::int64_t cols = 1;

  if (data.shape.size() == 1) {
    // 1D 向量 → 视为 [N, 1]
    rows = static_cast<std::int64_t>(data.shape[0]);
    cols = 1;
  } else if (data.shape.size() == 2) {
    rows = static_cast<std::int64_t>(data.shape[0]);
    cols = static_cast<std::int64_t>(data.shape[1]);
  } else {
    // 高维 → 展平后视为 [N, -1]
    rows = static_cast<std::int64_t>(data.shape[0]);
    cols = static_cast<std::int64_t>(total / data.shape[0]);
  }

  if (expected_cols > 0) {
    DOODLE_CHICK(cols == expected_cols, "npy 文件 {} 列数 {} 不匹配预期 {}", file_path.string(), cols, expected_cols);
  }

  Eigen::MatrixXf result(rows, cols);
  const float* ptr = data.data<float>();
  for (Eigen::Index r = 0; r < rows; ++r) {
    for (Eigen::Index c = 0; c < cols; ++c) {
      result(r, c) = ptr[static_cast<std::size_t>(r * cols + c)];
    }
  }
  return result;
}

void load_npy_if_exists(
    Eigen::MatrixXf& dest, const FSys::path& folder, const std::string& filename, std::int64_t expected_cols = -1
) {
  auto path = folder / filename;
  if (FSys::exists(path)) {
    dest = load_npy_matrix(path, expected_cols);
    SPDLOG_INFO("  已加载: {} ({}x{})", path.string(), dest.rows(), dest.cols());
  }
}

// 构建骨骼的 关节层级深度分组（对应 Python compute_idx_levels）
std::vector<std::vector<std::int64_t>> compute_joint_levels(const std::vector<std::int64_t>& parents) {
  std::vector<std::vector<std::int64_t>> levels;
  if (parents.empty()) return levels;

  const auto J = parents.size();

  // 第 0 层：根关节（父为 -1）
  levels.emplace_back();
  for (std::size_t i = 0; i < J; ++i) {
    if (parents[i] == -1) {
      levels[0].push_back(static_cast<std::int64_t>(i));
    }
  }

  // BFS 构建后续层级
  bool added;
  do {
    added = false;
    std::vector<std::int64_t> next_level;
    for (std::size_t i = 0; i < J; ++i) {
      if (parents[i] == -1) continue;  // 根已在第 0 层
      // 检查是否已在某层级中
      bool already_placed = false;
      for (const auto& lev : levels) {
        for (auto j : lev) {
          if (static_cast<std::size_t>(j) == i) {
            already_placed = true;
            break;
          }
        }
        if (already_placed) break;
      }
      if (already_placed) continue;

      // 检查父关节是否已在上一层级
      auto pi = static_cast<std::size_t>(parents[i]);
      bool parent_in_prev = false;
      if (!levels.empty()) {
        for (auto j : levels.back()) {
          if (static_cast<std::size_t>(j) == pi) {
            parent_in_prev = true;
            break;
          }
        }
      }
      if (parent_in_prev) {
        next_level.push_back(static_cast<std::int64_t>(i));
        added = true;
      }
    }
    if (!next_level.empty()) {
      levels.push_back(next_level);
    }
  } while (added);

  return levels;
}
}  // namespace

// ======================================================================
// 初始化
// ======================================================================

void skeleton_base::init_from_bone_hierarchy(
    const std::vector<std::pair<std::string, std::string>>& bone_hierarchy
) {
  bone_order_names_.clear();
  bone_index_.clear();
  joint_parents_.clear();

  // 构建 bone_order_names_ 和临时 parent_name 映射
  std::vector<std::string> parent_names;
  for (const auto& [name, parent] : bone_hierarchy) {
    bone_order_names_.push_back(name);
    parent_names.push_back(parent);
    // 插入索引映射（检查重复）
    DOODLE_CHICK(
        !bone_index_.contains(name), "重复的关节名称: {}", name
    );
    bone_index_[name] = static_cast<std::int64_t>(bone_order_names_.size() - 1);
  }

  nbjoints_ = static_cast<std::int64_t>(bone_order_names_.size());

  // 构建 joint_parents_ 索引数组
  joint_parents_.resize(static_cast<std::size_t>(nbjoints_));
  for (std::int64_t i = 0; i < nbjoints_; ++i) {
    const auto& pname = parent_names[static_cast<std::size_t>(i)];
    if (pname.empty()) {
      joint_parents_[static_cast<std::size_t>(i)] = -1;
    } else {
      auto it = bone_index_.find(pname);
      DOODLE_CHICK(it != bone_index_.end(), "关节 {} 的父关节 {} 不存在", bone_order_names_[static_cast<std::size_t>(i)], pname);
      joint_parents_[static_cast<std::size_t>(i)] = it->second;
    }
  }

  // 查找根关节索引
  root_idx_ = -1;
  for (std::int64_t i = 0; i < nbjoints_; ++i) {
    if (joint_parents_[static_cast<std::size_t>(i)] == -1) {
      DOODLE_CHICK(root_idx_ == -1, "存在多个根关节");
      root_idx_ = i;
    }
  }
  DOODLE_CHICK(root_idx_ != -1, "未找到根关节");

  // 构建 joint_levels_
  joint_levels_ = compute_joint_levels(joint_parents_);

  SPDLOG_INFO(
      "skeleton_base '{}' 初始化完成: {} 关节, 根索引={}, {} 层级",
      name_, nbjoints_, root_idx_, joint_levels_.size()
  );
}

// ======================================================================
// npy 数据加载
// ======================================================================

void skeleton_base::load_neutral_joints(const FSys::path& folder) {
  neutral_joints_ = load_npy_matrix(folder / "joints.npy", 3);
  DOODLE_CHICK(
      neutral_joints_.rows() == nbjoints_,
      "joints.npy 行数 {} 不匹配关节数 {}", neutral_joints_.rows(), nbjoints_
  );
  SPDLOG_INFO("  加载 neutral_joints: [{}x{}]", neutral_joints_.rows(), neutral_joints_.cols());
}

void skeleton_base::load_bvh_neutral_joints(const FSys::path& folder) {
  load_npy_if_exists(bvh_neutral_joints_, folder, "bvh_joints.npy", 3);
}

void skeleton_base::load_global_rot_offsets(const FSys::path& folder) {
  load_npy_if_exists(global_rot_offsets_, folder, "standard_t_pose_global_offsets_rots.npy");
  if (global_rot_offsets_.size() > 0) {
    DOODLE_CHICK(
        global_rot_offsets_.rows() == nbjoints_, "global_rot_offsets 行数 {} 不匹配关节数 {}",
        global_rot_offsets_.rows(), nbjoints_
    );
    DOODLE_CHICK(global_rot_offsets_.cols() == 9, "global_rot_offsets 列数应为 9，实际为 {}", global_rot_offsets_.cols());
  }
}

void skeleton_base::load_rest_pose_local_rot(const FSys::path& folder) {
  load_npy_if_exists(rest_pose_local_rot_, folder, "rest_pose_local_rot.npy");
  if (rest_pose_local_rot_.size() > 0) {
    DOODLE_CHICK(
        rest_pose_local_rot_.rows() == nbjoints_, "rest_pose_local_rot 行数 {} 不匹配关节数 {}",
        rest_pose_local_rot_.rows(), nbjoints_
    );
    DOODLE_CHICK(
        rest_pose_local_rot_.cols() == 9, "rest_pose_local_rot 列数应为 9，实际为 {}",
        rest_pose_local_rot_.cols()
    );
  }
}

void skeleton_base::load_relaxed_hands_rest_pose(const FSys::path& folder) {
  load_npy_if_exists(relaxed_hands_rest_pose_, folder, "relaxed_hands_rest_pose.npy");
}

void skeleton_base::load_all_from_folder(const FSys::path& folder) {
  SPDLOG_INFO("从文件夹加载骨骼数据: {}", folder.string());
  load_neutral_joints(folder);
  load_bvh_neutral_joints(folder);
  load_global_rot_offsets(folder);
  load_rest_pose_local_rot(folder);
  load_relaxed_hands_rest_pose(folder);
}

// ======================================================================
// 工厂函数：具体骨骼
// ======================================================================

namespace {

// 定义语义组（从骨骼层级中查找关节索引）
struct semantic_groups {
  std::vector<std::int64_t> left_foot;
  std::vector<std::int64_t> right_foot;
  std::vector<std::int64_t> left_hand;
  std::vector<std::int64_t> right_hand;
  std::vector<std::int64_t> hip;
};

semantic_groups resolve_semantic_groups(
    const skeleton_base& skel,
    const std::vector<std::string>& left_foot_names,
    const std::vector<std::string>& right_foot_names,
    const std::vector<std::string>& left_hand_names,
    const std::vector<std::string>& right_hand_names,
    const std::vector<std::string>& hip_names
) {
  semantic_groups g;
  auto resolve = [&](const std::vector<std::string>& names) {
    std::vector<std::int64_t> indices;
    indices.reserve(names.size());
    for (const auto& n : names) {
      auto it = skel.bone_index_.find(n);
      DOODLE_CHICK(it != skel.bone_index_.end(), "关节名称 '{}' 未在骨骼中找到", n);
      indices.push_back(it->second);
    }
    return indices;
  };
  g.left_foot   = resolve(left_foot_names);
  g.right_foot  = resolve(right_foot_names);
  g.left_hand   = resolve(left_hand_names);
  g.right_hand  = resolve(right_hand_names);
  g.hip         = resolve(hip_names);
  return g;
}

void apply_semantic_groups(skeleton_base& skel, const semantic_groups& g) {
  skel.left_foot_joint_idx_  = g.left_foot;
  skel.right_foot_joint_idx_ = g.right_foot;
  skel.left_hand_joint_idx_  = g.left_hand;
  skel.right_hand_joint_idx_ = g.right_hand;
  skel.hip_joint_idx_        = g.hip;

  // foot_joint_idx = left + right
  skel.foot_joint_idx_.clear();
  skel.foot_joint_idx_.insert(
      skel.foot_joint_idx_.end(), g.left_foot.begin(), g.left_foot.end()
  );
  skel.foot_joint_idx_.insert(
      skel.foot_joint_idx_.end(), g.right_foot.begin(), g.right_foot.end()
  );
}

}  // namespace

// ======================================================================
// SOMASkeleton30
// ======================================================================

std::shared_ptr<skeleton_base> skeleton_base::create_soma_skeleton_30(const FSys::path& folder) {
  auto skel = std::make_shared<skeleton_base>();
  skel->name_ = "somaskel30";

  skel->init_from_bone_hierarchy({
      {"Hips", ""},
      {"Spine1", "Hips"},
      {"Spine2", "Spine1"},
      {"Chest", "Spine2"},
      {"Neck1", "Chest"},
      {"Neck2", "Neck1"},
      {"Head", "Neck2"},
      {"Jaw", "Head"},
      {"LeftEye", "Head"},
      {"RightEye", "Head"},
      {"LeftShoulder", "Chest"},
      {"LeftArm", "LeftShoulder"},
      {"LeftForeArm", "LeftArm"},
      {"LeftHand", "LeftForeArm"},
      {"LeftHandThumbEnd", "LeftHand"},
      {"LeftHandMiddleEnd", "LeftHand"},
      {"RightShoulder", "Chest"},
      {"RightArm", "RightShoulder"},
      {"RightForeArm", "RightArm"},
      {"RightHand", "RightForeArm"},
      {"RightHandThumbEnd", "RightHand"},
      {"RightHandMiddleEnd", "RightHand"},
      {"LeftLeg", "Hips"},
      {"LeftShin", "LeftLeg"},
      {"LeftFoot", "LeftShin"},
      {"LeftToeBase", "LeftFoot"},
      {"RightLeg", "Hips"},
      {"RightShin", "RightLeg"},
      {"RightFoot", "RightShin"},
      {"RightToeBase", "RightFoot"},
  });

  auto g = resolve_semantic_groups(
      *skel,
      {"LeftFoot", "LeftToeBase"},               // left foot
      {"RightFoot", "RightToeBase"},              // right foot
      {"LeftHand", "LeftHandMiddleEnd"},          // left hand
      {"RightHand", "RightHandMiddleEnd"},        // right hand
      {"RightLeg", "LeftLeg"}                     // hip [right, left]
  );
  apply_semantic_groups(*skel, g);

  if (!folder.empty()) {
    skel->load_all_from_folder(folder);
  }

  return skel;
}

// ======================================================================
// SOMASkeleton77
// ======================================================================

std::shared_ptr<skeleton_base> skeleton_base::create_soma_skeleton_77(const FSys::path& folder) {
  auto skel = std::make_shared<skeleton_base>();
  skel->name_ = "somaskel77";

  skel->init_from_bone_hierarchy({
      {"Hips", ""},
      {"Spine1", "Hips"},
      {"Spine2", "Spine1"},
      {"Chest", "Spine2"},
      {"Neck1", "Chest"},
      {"Neck2", "Neck1"},
      {"Head", "Neck2"},
      {"HeadEnd", "Head"},
      {"Jaw", "Head"},
      {"LeftEye", "Head"},
      {"RightEye", "Head"},
      {"LeftShoulder", "Chest"},
      {"LeftArm", "LeftShoulder"},
      {"LeftForeArm", "LeftArm"},
      {"LeftHand", "LeftForeArm"},
      {"LeftHandThumb1", "LeftHand"},
      {"LeftHandThumb2", "LeftHandThumb1"},
      {"LeftHandThumb3", "LeftHandThumb2"},
      {"LeftHandThumbEnd", "LeftHandThumb3"},
      {"LeftHandIndex1", "LeftHand"},
      {"LeftHandIndex2", "LeftHandIndex1"},
      {"LeftHandIndex3", "LeftHandIndex2"},
      {"LeftHandIndex4", "LeftHandIndex3"},
      {"LeftHandIndexEnd", "LeftHandIndex4"},
      {"LeftHandMiddle1", "LeftHand"},
      {"LeftHandMiddle2", "LeftHandMiddle1"},
      {"LeftHandMiddle3", "LeftHandMiddle2"},
      {"LeftHandMiddle4", "LeftHandMiddle3"},
      {"LeftHandMiddleEnd", "LeftHandMiddle4"},
      {"LeftHandRing1", "LeftHand"},
      {"LeftHandRing2", "LeftHandRing1"},
      {"LeftHandRing3", "LeftHandRing2"},
      {"LeftHandRing4", "LeftHandRing3"},
      {"LeftHandRingEnd", "LeftHandRing4"},
      {"LeftHandPinky1", "LeftHand"},
      {"LeftHandPinky2", "LeftHandPinky1"},
      {"LeftHandPinky3", "LeftHandPinky2"},
      {"LeftHandPinky4", "LeftHandPinky3"},
      {"LeftHandPinkyEnd", "LeftHandPinky4"},
      {"RightShoulder", "Chest"},
      {"RightArm", "RightShoulder"},
      {"RightForeArm", "RightArm"},
      {"RightHand", "RightForeArm"},
      {"RightHandThumb1", "RightHand"},
      {"RightHandThumb2", "RightHandThumb1"},
      {"RightHandThumb3", "RightHandThumb2"},
      {"RightHandThumbEnd", "RightHandThumb3"},
      {"RightHandIndex1", "RightHand"},
      {"RightHandIndex2", "RightHandIndex1"},
      {"RightHandIndex3", "RightHandIndex2"},
      {"RightHandIndex4", "RightHandIndex3"},
      {"RightHandIndexEnd", "RightHandIndex4"},
      {"RightHandMiddle1", "RightHand"},
      {"RightHandMiddle2", "RightHandMiddle1"},
      {"RightHandMiddle3", "RightHandMiddle2"},
      {"RightHandMiddle4", "RightHandMiddle3"},
      {"RightHandMiddleEnd", "RightHandMiddle4"},
      {"RightHandRing1", "RightHand"},
      {"RightHandRing2", "RightHandRing1"},
      {"RightHandRing3", "RightHandRing2"},
      {"RightHandRing4", "RightHandRing3"},
      {"RightHandRingEnd", "RightHandRing4"},
      {"RightHandPinky1", "RightHand"},
      {"RightHandPinky2", "RightHandPinky1"},
      {"RightHandPinky3", "RightHandPinky2"},
      {"RightHandPinky4", "RightHandPinky3"},
      {"RightHandPinkyEnd", "RightHandPinky4"},
      {"LeftLeg", "Hips"},
      {"LeftShin", "LeftLeg"},
      {"LeftFoot", "LeftShin"},
      {"LeftToeBase", "LeftFoot"},
      {"LeftToeEnd", "LeftToeBase"},
      {"RightLeg", "Hips"},
      {"RightShin", "RightLeg"},
      {"RightFoot", "RightShin"},
      {"RightToeBase", "RightFoot"},
      {"RightToeEnd", "RightToeBase"},
  });

  auto g = resolve_semantic_groups(
      *skel,
      {"LeftFoot", "LeftToeBase", "LeftToeEnd"},
      {"RightFoot", "RightToeBase", "RightToeEnd"},
      {"LeftHand", "LeftHandThumb1", "LeftHandThumb2", "LeftHandThumb3", "LeftHandThumbEnd",
       "LeftHandIndex1", "LeftHandIndex2", "LeftHandIndex3", "LeftHandIndex4", "LeftHandIndexEnd",
       "LeftHandMiddle1", "LeftHandMiddle2", "LeftHandMiddle3", "LeftHandMiddle4", "LeftHandMiddleEnd",
       "LeftHandRing1", "LeftHandRing2", "LeftHandRing3", "LeftHandRing4", "LeftHandRingEnd",
       "LeftHandPinky1", "LeftHandPinky2", "LeftHandPinky3", "LeftHandPinky4", "LeftHandPinkyEnd"},
      {"RightHand", "RightHandThumb1", "RightHandThumb2", "RightHandThumb3", "RightHandThumbEnd",
       "RightHandIndex1", "RightHandIndex2", "RightHandIndex3", "RightHandIndex4", "RightHandIndexEnd",
       "RightHandMiddle1", "RightHandMiddle2", "RightHandMiddle3", "RightHandMiddle4", "RightHandMiddleEnd",
       "RightHandRing1", "RightHandRing2", "RightHandRing3", "RightHandRing4", "RightHandRingEnd",
       "RightHandPinky1", "RightHandPinky2", "RightHandPinky3", "RightHandPinky4", "RightHandPinkyEnd"},
      {"RightLeg", "LeftLeg"}
  );
  apply_semantic_groups(*skel, g);

  if (!folder.empty()) {
    skel->load_all_from_folder(folder);
  }

  return skel;
}

// ======================================================================
// G1Skeleton34
// ======================================================================

std::shared_ptr<skeleton_base> skeleton_base::create_g1_skeleton_34(const FSys::path& folder) {
  auto skel = std::make_shared<skeleton_base>();
  skel->name_ = "g1skel34";

  skel->init_from_bone_hierarchy({
      {"pelvis_skel", ""},
      {"left_hip_pitch_skel", "pelvis_skel"},
      {"left_hip_roll_skel", "left_hip_pitch_skel"},
      {"left_hip_yaw_skel", "left_hip_roll_skel"},
      {"left_knee_skel", "left_hip_yaw_skel"},
      {"left_ankle_pitch_skel", "left_knee_skel"},
      {"left_ankle_roll_skel", "left_ankle_pitch_skel"},
      {"left_toe_base", "left_ankle_roll_skel"},
      {"right_hip_pitch_skel", "pelvis_skel"},
      {"right_hip_roll_skel", "right_hip_pitch_skel"},
      {"right_hip_yaw_skel", "right_hip_roll_skel"},
      {"right_knee_skel", "right_hip_yaw_skel"},
      {"right_ankle_pitch_skel", "right_knee_skel"},
      {"right_ankle_roll_skel", "right_ankle_pitch_skel"},
      {"right_toe_base", "right_ankle_roll_skel"},
      {"waist_yaw_skel", "pelvis_skel"},
      {"waist_roll_skel", "waist_yaw_skel"},
      {"waist_pitch_skel", "waist_roll_skel"},
      {"left_shoulder_pitch_skel", "waist_pitch_skel"},
      {"left_shoulder_roll_skel", "left_shoulder_pitch_skel"},
      {"left_shoulder_yaw_skel", "left_shoulder_roll_skel"},
      {"left_elbow_skel", "left_shoulder_yaw_skel"},
      {"left_wrist_roll_skel", "left_elbow_skel"},
      {"left_wrist_pitch_skel", "left_wrist_roll_skel"},
      {"left_wrist_yaw_skel", "left_wrist_pitch_skel"},
      {"left_hand_roll_skel", "left_wrist_yaw_skel"},
      {"right_shoulder_pitch_skel", "waist_pitch_skel"},
      {"right_shoulder_roll_skel", "right_shoulder_pitch_skel"},
      {"right_shoulder_yaw_skel", "right_shoulder_roll_skel"},
      {"right_elbow_skel", "right_shoulder_yaw_skel"},
      {"right_wrist_roll_skel", "right_elbow_skel"},
      {"right_wrist_pitch_skel", "right_wrist_roll_skel"},
      {"right_wrist_yaw_skel", "right_wrist_pitch_skel"},
      {"right_hand_roll_skel", "right_wrist_yaw_skel"},
  });

  auto g = resolve_semantic_groups(
      *skel,
      {"left_ankle_roll_skel", "left_toe_base"},
      {"right_ankle_roll_skel", "right_toe_base"},
      {"left_wrist_yaw_skel", "left_hand_roll_skel"},
      {"right_wrist_yaw_skel", "right_hand_roll_skel"},
      {"right_hip_pitch_skel", "left_hip_pitch_skel"}
  );
  apply_semantic_groups(*skel, g);

  if (!folder.empty()) {
    skel->load_all_from_folder(folder);
  }

  return skel;
}

// ======================================================================
// SMPLXSkeleton22
// ======================================================================

std::shared_ptr<skeleton_base> skeleton_base::create_smplx_skeleton_22(const FSys::path& folder) {
  auto skel = std::make_shared<skeleton_base>();
  skel->name_ = "smplx22";

  skel->init_from_bone_hierarchy({
      {"pelvis", ""},
      {"left_hip", "pelvis"},
      {"right_hip", "pelvis"},
      {"spine1", "pelvis"},
      {"left_knee", "left_hip"},
      {"right_knee", "right_hip"},
      {"spine2", "spine1"},
      {"left_ankle", "left_knee"},
      {"right_ankle", "right_knee"},
      {"spine3", "spine2"},
      {"left_foot", "left_ankle"},
      {"right_foot", "right_ankle"},
      {"neck", "spine3"},
      {"left_collar", "spine3"},
      {"right_collar", "spine3"},
      {"head", "neck"},
      {"left_shoulder", "left_collar"},
      {"right_shoulder", "right_collar"},
      {"left_elbow", "left_shoulder"},
      {"right_elbow", "right_shoulder"},
      {"left_wrist", "left_elbow"},
      {"right_wrist", "right_elbow"},
  });

  auto g = resolve_semantic_groups(
      *skel,
      {"left_ankle", "left_foot"},
      {"right_ankle", "right_foot"},
      {"left_wrist"},
      {"right_wrist"},
      {"right_hip", "left_hip"}
  );
  apply_semantic_groups(*skel, g);

  if (!folder.empty()) {
    skel->load_all_from_folder(folder);
  }

  return skel;
}

}  // namespace doodle::ai
