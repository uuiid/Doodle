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
// 辅助：矩阵操作（3x3 ↔ 4x4 转换）
// ======================================================================
namespace {

void matrix_9_to_4x4(const float* src_9, Eigen::Matrix4f& dst) {
  dst.setIdentity();
  dst(0, 0) = src_9[0];
  dst(0, 1) = src_9[1];
  dst(0, 2) = src_9[2];
  dst(1, 0) = src_9[3];
  dst(1, 1) = src_9[4];
  dst(1, 2) = src_9[5];
  dst(2, 0) = src_9[6];
  dst(2, 1) = src_9[7];
  dst(2, 2) = src_9[8];
}

void matrix_4x4_to_9(const Eigen::Matrix4f& src, float* dst_9) {
  dst_9[0] = src(0, 0);
  dst_9[1] = src(0, 1);
  dst_9[2] = src(0, 2);
  dst_9[3] = src(1, 0);
  dst_9[4] = src(1, 1);
  dst_9[5] = src(1, 2);
  dst_9[6] = src(2, 0);
  dst_9[7] = src(2, 1);
  dst_9[8] = src(2, 2);
}

}  // namespace

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

  DOODLE_CHICK(data.word_size == sizeof(float), "npy 文件 {} 数据类型不是 float32", file_path.string());
  DOODLE_CHICK(data.fortran_order == false, "npy 文件 {} 不是 C order (row-major)", file_path.string());

  Eigen::Map<const Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>> map(
      data.data<float>(), rows, cols
  );

  Eigen::MatrixXf result{rows, cols};
  result = map;
  return result;
}

bool load_npy_if_exists(
    Eigen::MatrixXf& dest, const FSys::path& folder, const std::string& filename, std::int64_t expected_cols = -1
) {
  auto path = folder / filename;
  if (FSys::exists(path)) {
    dest = load_npy_matrix(path, expected_cols);
    SPDLOG_INFO("  已加载: {} ({}x{})", path.string(), dest.rows(), dest.cols());
    return true;
  }
  return false;
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
      auto pi             = static_cast<std::size_t>(parents[i]);
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

void skeleton_base::init_from_bone_hierarchy(const std::vector<std::pair<std::string, std::string>>& bone_hierarchy) {
  bone_order_names_.clear();
  bone_index_.clear();
  joint_parents_.clear();

  // 构建 bone_order_names_ 和临时 parent_name 映射
  std::vector<std::string> parent_names;
  for (const auto& [name, parent] : bone_hierarchy) {
    bone_order_names_.push_back(name);
    parent_names.push_back(parent);
    // 插入索引映射（检查重复）
    DOODLE_CHICK(!bone_index_.contains(name), "重复的关节名称: {}", name);
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
      DOODLE_CHICK(
          it != bone_index_.end(), "关节 {} 的父关节 {} 不存在", bone_order_names_[static_cast<std::size_t>(i)], pname
      );
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
      "skeleton_base '{}' 初始化完成: {} 关节, 根索引={}, {} 层级", name_, nbjoints_, root_idx_, joint_levels_.size()
  );
}

// ======================================================================
// npy 数据加载
// ======================================================================

void skeleton_base::load_neutral_joints(const FSys::path& folder) {
  neutral_joints_ = load_npy_matrix(folder / "neutral_joints.npy", 3);
  DOODLE_CHICK(
      neutral_joints_.rows() == nbjoints_, "neutral_joints.npy 行数 {} 不匹配关节数 {}", neutral_joints_.rows(),
      nbjoints_
  );
  SPDLOG_INFO("  加载 neutral_joints: [{}x{}]", neutral_joints_.rows(), neutral_joints_.cols());
}

void skeleton_base::load_bvh_neutral_joints(const FSys::path& folder) {
  if (load_npy_if_exists(bvh_neutral_joints_, folder, "bvh_neutral_joints.npy", 3))
    SPDLOG_INFO("  加载 bvh_neutral_joints: [{}x{}]", bvh_neutral_joints_.rows(), bvh_neutral_joints_.cols());
}

void skeleton_base::load_global_rot_offsets(const FSys::path& folder) {
  if (load_npy_if_exists(global_rot_offsets_, folder, "global_rot_offsets.npy")) {
    DOODLE_CHICK(
        global_rot_offsets_.rows() == nbjoints_, "global_rot_offsets 行数 {} 不匹配关节数 {}",
        global_rot_offsets_.rows(), nbjoints_
    );
    DOODLE_CHICK(
        global_rot_offsets_.cols() == 9, "global_rot_offsets 列数应为 9，实际为 {}", global_rot_offsets_.cols()
    );
  }
}

void skeleton_base::load_rest_pose_local_rot(const FSys::path& folder) {
  if (load_npy_if_exists(rest_pose_local_rot_, folder, "rest_pose_local_rot.npy")) {
    DOODLE_CHICK(
        rest_pose_local_rot_.rows() == nbjoints_, "rest_pose_local_rot 行数 {} 不匹配关节数 {}",
        rest_pose_local_rot_.rows(), nbjoints_
    );
    DOODLE_CHICK(
        rest_pose_local_rot_.cols() == 9, "rest_pose_local_rot 列数应为 9，实际为 {}", rest_pose_local_rot_.cols()
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
    const skeleton_base& skel, const std::vector<std::string>& left_foot_names,
    const std::vector<std::string>& right_foot_names, const std::vector<std::string>& left_hand_names,
    const std::vector<std::string>& right_hand_names, const std::vector<std::string>& hip_names
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
  g.left_foot  = resolve(left_foot_names);
  g.right_foot = resolve(right_foot_names);
  g.left_hand  = resolve(left_hand_names);
  g.right_hand = resolve(right_hand_names);
  g.hip        = resolve(hip_names);
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
  skel.foot_joint_idx_.insert(skel.foot_joint_idx_.end(), g.left_foot.begin(), g.left_foot.end());
  skel.foot_joint_idx_.insert(skel.foot_joint_idx_.end(), g.right_foot.begin(), g.right_foot.end());
}

}  // namespace

// ======================================================================
// SOMASkeleton30
// ======================================================================

std::shared_ptr<skeleton_base> skeleton_base::create_soma_skeleton_30(
    const FSys::path& folder, const FSys::path& in_77_folder
) {
  DOODLE_CHICK(!folder.empty(), "创建 SOMASkeleton30 时必须提供 folder 路径");

  auto skel   = std::make_shared<skeleton_base>();
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
      *skel, {"LeftFoot", "LeftToeBase"},   // left foot
      {"RightFoot", "RightToeBase"},        // right foot
      {"LeftHand", "LeftHandMiddleEnd"},    // left hand
      {"RightHand", "RightHandMiddleEnd"},  // right hand
      {"RightLeg", "LeftLeg"}               // hip [right, left]
  );
  apply_semantic_groups(*skel, g);
  skel->load_all_from_folder(folder);

  return skel;
}

// ======================================================================
// SOMASkeleton77
// ======================================================================

std::shared_ptr<skeleton_base> skeleton_base::create_soma_skeleton_77(const FSys::path& folder) {
  auto skel   = std::make_shared<skeleton_base>();
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
      *skel, {"LeftFoot", "LeftToeBase", "LeftToeEnd"}, {"RightFoot", "RightToeBase", "RightToeEnd"},
      {"LeftHand",        "LeftHandThumb1",  "LeftHandThumb2",  "LeftHandThumb3",  "LeftHandThumbEnd",
       "LeftHandIndex1",  "LeftHandIndex2",  "LeftHandIndex3",  "LeftHandIndex4",  "LeftHandIndexEnd",
       "LeftHandMiddle1", "LeftHandMiddle2", "LeftHandMiddle3", "LeftHandMiddle4", "LeftHandMiddleEnd",
       "LeftHandRing1",   "LeftHandRing2",   "LeftHandRing3",   "LeftHandRing4",   "LeftHandRingEnd",
       "LeftHandPinky1",  "LeftHandPinky2",  "LeftHandPinky3",  "LeftHandPinky4",  "LeftHandPinkyEnd"},
      {"RightHand",        "RightHandThumb1",  "RightHandThumb2",  "RightHandThumb3",  "RightHandThumbEnd",
       "RightHandIndex1",  "RightHandIndex2",  "RightHandIndex3",  "RightHandIndex4",  "RightHandIndexEnd",
       "RightHandMiddle1", "RightHandMiddle2", "RightHandMiddle3", "RightHandMiddle4", "RightHandMiddleEnd",
       "RightHandRing1",   "RightHandRing2",   "RightHandRing3",   "RightHandRing4",   "RightHandRingEnd",
       "RightHandPinky1",  "RightHandPinky2",  "RightHandPinky3",  "RightHandPinky4",  "RightHandPinkyEnd"},
      {"RightLeg", "LeftLeg"}
  );
  apply_semantic_groups(*skel, g);

  if (!folder.empty()) {
    skel->load_all_from_folder(folder);
  }

  return skel;
}

// ======================================================================
// FK 成员方法
// ======================================================================

skeleton_base::fk_result skeleton_base::fk(
    const Eigen::MatrixXf& local_rot_mats, const Eigen::MatrixXf& root_positions
) const {
  const Eigen::Index total_frames = local_rot_mats.rows();
  const Eigen::Index J            = nbjoints_;
  DOODLE_CHICK(local_rot_mats.cols() == J * 9, "local_rot_mats 列数 {} 不匹配 J*9 = {}", local_rot_mats.cols(), J * 9);
  DOODLE_CHICK(root_positions.rows() == total_frames, "root_positions 行数不匹配");
  DOODLE_CHICK(root_positions.cols() == 3, "root_positions 列数 != 3");

  fk_result result;
  result.global_rot_mats.resize(total_frames, J * 9);
  result.posed_joints.resize(total_frames, J * 3);
  result.posed_joints_norootpos.resize(total_frames, J * 3);

  DOODLE_CHICK(!joint_levels_.empty(), "skeleton_base.joint_levels_ 为空，请先调用 init_from_bone_hierarchy");

  for (Eigen::Index f = 0; f < total_frames; ++f) {
    const float* rot_row = local_rot_mats.row(f).data();
    const float* pos_row = root_positions.row(f).data();

    std::vector<Eigen::Matrix4f> transforms(static_cast<std::size_t>(J));

    // 根关节
    const auto root_i = static_cast<std::size_t>(root_idx_);
    {
      Eigen::Matrix4f local_T = Eigen::Matrix4f::Identity();
      matrix_9_to_4x4(rot_row + root_i * 9, local_T);
      transforms[root_i] = local_T;
      transforms[root_i](0, 3) += pos_row[0];
      transforms[root_i](1, 3) += pos_row[1];
      transforms[root_i](2, 3) += pos_row[2];
    }

    // 逐层级计算
    for (const auto& level : joint_levels_) {
      for (const auto& j_idx : level) {
        if (j_idx == root_idx_) continue;
        const auto ji = static_cast<std::size_t>(j_idx);
        const auto pi = static_cast<std::size_t>(joint_parents_[ji]);

        // 相对位置 = neutral_joints[j] - neutral_joints[parent(j)]
        Eigen::Vector3f rel_joint;
        rel_joint(0)            = neutral_joints_(j_idx, 0) - neutral_joints_(static_cast<Eigen::Index>(pi), 0);
        rel_joint(1)            = neutral_joints_(j_idx, 1) - neutral_joints_(static_cast<Eigen::Index>(pi), 1);
        rel_joint(2)            = neutral_joints_(j_idx, 2) - neutral_joints_(static_cast<Eigen::Index>(pi), 2);

        Eigen::Matrix4f local_T = Eigen::Matrix4f::Identity();
        matrix_9_to_4x4(rot_row + j_idx * 9, local_T);
        local_T(0, 3)  = rel_joint(0);
        local_T(1, 3)  = rel_joint(1);
        local_T(2, 3)  = rel_joint(2);

        transforms[ji] = transforms[pi] * local_T;
      }
    }

    // 提取结果
    float* global_rot_out = result.global_rot_mats.row(f).data();
    float* posed_out      = result.posed_joints.row(f).data();
    float* posed_no_root  = result.posed_joints_norootpos.row(f).data();

    for (Eigen::Index j = 0; j < J; ++j) {
      const auto ji = static_cast<std::size_t>(j);
      matrix_4x4_to_9(transforms[ji], global_rot_out + j * 9);

      posed_out[j * 3 + 0]     = transforms[ji](0, 3);
      posed_out[j * 3 + 1]     = transforms[ji](1, 3);
      posed_out[j * 3 + 2]     = transforms[ji](2, 3);

      posed_no_root[j * 3 + 0] = transforms[ji](0, 3) - pos_row[0];
      posed_no_root[j * 3 + 1] = transforms[ji](1, 3) - pos_row[1];
      posed_no_root[j * 3 + 2] = transforms[ji](2, 3) - pos_row[2];
    }
  }

  return result;
}

// ======================================================================
// 全局旋转 → 局部旋转 成员方法
// ======================================================================

Eigen::MatrixXf skeleton_base::global_rots_to_local_rots(const Eigen::MatrixXf& global_rot_mats) const {
  const Eigen::Index total = global_rot_mats.rows();
  const Eigen::Index J     = nbjoints_;
  DOODLE_CHICK(global_rot_mats.cols() == J * 9, "global_rot_mats 列数不匹配");

  Eigen::MatrixXf local_rot_mats(total, J * 9);

  for (Eigen::Index f = 0; f < total; ++f) {
    const float* global_row = global_rot_mats.row(f).data();
    float* local_row        = local_rot_mats.row(f).data();

    for (Eigen::Index j = 0; j < J; ++j) {
      const auto ji = static_cast<std::size_t>(j);

      Eigen::Matrix3f R_global;
      R_global(0, 0) = global_row[j * 9 + 0];
      R_global(0, 1) = global_row[j * 9 + 1];
      R_global(0, 2) = global_row[j * 9 + 2];
      R_global(1, 0) = global_row[j * 9 + 3];
      R_global(1, 1) = global_row[j * 9 + 4];
      R_global(1, 2) = global_row[j * 9 + 5];
      R_global(2, 0) = global_row[j * 9 + 6];
      R_global(2, 1) = global_row[j * 9 + 7];
      R_global(2, 2) = global_row[j * 9 + 8];

      Eigen::Matrix3f R_local;

      if (j == root_idx_) {
        R_local = R_global;
      } else {
        const auto pi = static_cast<std::size_t>(joint_parents_[ji]);

        Eigen::Matrix3f R_parent;
        R_parent(0, 0) = global_row[pi * 9 + 0];
        R_parent(0, 1) = global_row[pi * 9 + 1];
        R_parent(0, 2) = global_row[pi * 9 + 2];
        R_parent(1, 0) = global_row[pi * 9 + 3];
        R_parent(1, 1) = global_row[pi * 9 + 4];
        R_parent(1, 2) = global_row[pi * 9 + 5];
        R_parent(2, 0) = global_row[pi * 9 + 6];
        R_parent(2, 1) = global_row[pi * 9 + 7];
        R_parent(2, 2) = global_row[pi * 9 + 8];

        R_local        = R_parent.transpose() * R_global;
      }

      local_row[j * 9 + 0] = R_local(0, 0);
      local_row[j * 9 + 1] = R_local(0, 1);
      local_row[j * 9 + 2] = R_local(0, 2);
      local_row[j * 9 + 3] = R_local(1, 0);
      local_row[j * 9 + 4] = R_local(1, 1);
      local_row[j * 9 + 5] = R_local(1, 2);
      local_row[j * 9 + 6] = R_local(2, 0);
      local_row[j * 9 + 7] = R_local(2, 1);
      local_row[j * 9 + 8] = R_local(2, 2);
    }
  }

  return local_rot_mats;
}

// ======================================================================
// get_skel_slice
// ======================================================================

std::vector<std::int64_t> skeleton_base::get_skel_slice(const skeleton_base& target) const {
  std::vector<std::int64_t> slice;
  slice.reserve(bone_order_names_.size());
  for (const auto& name : bone_order_names_) {
    auto it = target.bone_index_.find(name);
    DOODLE_CHICK(it != target.bone_index_.end(), "关节 '{}' 不在目标骨骼 '{}' 中", name, target.name_);
    slice.push_back(it->second);
  }
  return slice;
}

// ======================================================================
// to_soma_skeleton_77（SOMA30 → SOMA77 关节扩展）
// ======================================================================

Eigen::MatrixXf skeleton_base::to_soma_skeleton_77(const Eigen::MatrixXf& local_joint_rots_subset) const {
  // local_joint_rots_subset size = [B * T, 30*9]

  DOODLE_CHICK(name_ == "somaskel30", "to_soma_skeleton_77 仅用于 somaskel30，当前为 '{}'", name_);

  // 懒加载 SOMA77 骨骼
  if (!somaskel77_cache_) {
    somaskel77_cache_ = create_soma_skeleton_77();
  }
  const auto& skel77              = *somaskel77_cache_;

  const Eigen::Index total_frames = local_joint_rots_subset.rows();
  const Eigen::Index J77          = 77;

  // 验证 relaxed_hands_rest_pose 已加载
  DOODLE_CHICK(
      skel77.relaxed_hands_rest_pose_.size() > 0,
      "SOMA77 未加载 relaxed_hands_rest_pose —— 请先调用 load_relaxed_hands_rest_pose 或提供有效文件夹"
  );
  DOODLE_CHICK(
      skel77.relaxed_hands_rest_pose_.rows() == J77, "relaxed_hands_rest_pose 行数 {} != 77",
      skel77.relaxed_hands_rest_pose_.rows()
  );
  DOODLE_CHICK(
      skel77.relaxed_hands_rest_pose_.cols() == 9, "relaxed_hands_rest_pose 列数 {} != 9",
      skel77.relaxed_hands_rest_pose_.cols()
  );

  // 将 relaxed_hands_rest_pose [77, 9] 重复 total_frames 次 → [BT, 77*9]
  Eigen::MatrixXf result(total_frames, J77 * 9);
  for (Eigen::Index f = 0; f < total_frames; ++f) {
    Eigen::Map<Eigen::Matrix<float, 1, Eigen::Dynamic, Eigen::RowMajor>> row(result.row(f).data(), J77 * 9);
    row = Eigen::Map<const Eigen::Matrix<float, 1, Eigen::Dynamic, Eigen::RowMajor>>(
        skel77.relaxed_hands_rest_pose_.data(), J77 * 9
    );
  }

  // 获取 SOMA30 → SOMA77 索引映射，填入 30 关节数据
  auto skel_slice = get_skel_slice(skel77);
  for (Eigen::Index f = 0; f < total_frames; ++f) {
    for (std::size_t j = 0; j < skel_slice.size(); ++j) {
      const auto idx77       = skel_slice[j];
      const Eigen::Index j30 = static_cast<Eigen::Index>(j);
      for (int k = 0; k < 9; ++k) {
        result(f, idx77 * 9 + k) = local_joint_rots_subset(f, j30 * 9 + k);
      }
    }
  }

  return result;
}

// ======================================================================
// output_to_soma_skeleton_77
// ======================================================================

skeleton_base::output_77_result skeleton_base::output_to_soma_skeleton_77(
    const Eigen::MatrixXf& local_rot_mats, const Eigen::MatrixXf& root_positions,
    const std::optional<Eigen::MatrixXf>& foot_contacts
) const {
  // 1. 扩展局部旋转至 77 关节
  auto local_rot_mats_77 = to_soma_skeleton_77(local_rot_mats);

  // 2. 在 SOMA77 上运行 FK
  DOODLE_CHICK(
      somaskel77_cache_ != nullptr, "output_to_soma_skeleton_77: SOMA77 骨骼未创建 —— to_soma_skeleton_77 应已创建"
  );
  auto fk_res = somaskel77_cache_->fk(local_rot_mats_77, root_positions);

  // 3. 组装结果
  output_77_result result;
  result.local_rot_mats  = std::move(local_rot_mats_77);
  result.global_rot_mats = std::move(fk_res.global_rot_mats);
  result.posed_joints    = std::move(fk_res.posed_joints);

  // 4. 如果有 foot_contacts，从 4 通道扩展为 6 通道
  //    输入 [..., 4]: [L_heel, L_toe, R_heel, R_toe]
  //    输出 [..., 6]: [L_heel, L_toe, L_toe_end, R_heel, R_toe, R_toe_end]
  //    toe_end 复制 toe_base 的值
  if (foot_contacts.has_value()) {
    const auto& fc           = foot_contacts.value();
    const Eigen::Index nrows = fc.rows();
    DOODLE_CHICK(fc.cols() == 4, "foot_contacts 列数应为 4，实际为 {}", fc.cols());

    Eigen::MatrixXf fc_77(nrows, 6);
    for (Eigen::Index i = 0; i < nrows; ++i) {
      // L_heel = fc[0]
      fc_77(i, 0) = fc(i, 0);
      // L_toe = fc[1]
      fc_77(i, 1) = fc(i, 1);
      // L_toe_end = fc[1] (复制 L_toe)
      fc_77(i, 2) = fc(i, 1);
      // R_heel = fc[2]
      fc_77(i, 3) = fc(i, 2);
      // R_toe = fc[3]
      fc_77(i, 4) = fc(i, 3);
      // R_toe_end = fc[3] (复制 R_toe)
      fc_77(i, 5) = fc(i, 3);
    }
    result.foot_contacts = std::move(fc_77);
  }

  return result;
}

}  // namespace doodle::ai
