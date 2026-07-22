//
// Created by TD on 25-7-22.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <Eigen/Dense>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace doodle::ai {

// ======================================================================
// 骨骼基类（对应 Python SkeletonBase）
// ======================================================================

/// @brief 骨骼基类，存储关节层级、中性姿态、语义标签和运动学辅助方法。
///
/// 子类通过静态 bone_order_names_with_parents 定义关节布局。
/// 数据文件（joints.npy 等）以 .npy 格式从指定文件夹加载。
///
/// 用法：
/// @code
///   auto skel = skeleton_base::create_soma_skeleton_30("path/to/somaskel30");
///   // skel 为 std::shared_ptr<skeleton_base>
/// @endcode
class skeleton_base {
 public:
  // ======================================================================
  // 关节层级（由子类构造函数通过 init_from_bone_hierarchy 或 npy 构建）
  // ======================================================================

  std::string name_;         ///< 骨骼名称（如 "somaskel30"）
  std::int64_t nbjoints_{};  ///< 关节数量
  std::int64_t root_idx_{};  ///< 根关节索引

  std::vector<std::string> bone_order_names_;                 ///< [J] 关节名称列表（按顺序）
  std::unordered_map<std::string, std::int64_t> bone_index_;  ///< 关节名称 → 索引映射
  std::vector<std::int64_t> joint_parents_;                   ///< [J] 父关节索引，根关节为 -1

  // ======================================================================
  // 关节数据（从 .npy 文件加载）
  // ======================================================================

  Eigen::MatrixXf neutral_joints_;       ///< [J, 3] 中性姿态关节位置
  Eigen::MatrixXf bvh_neutral_joints_;   ///< [J, 3] BVH 中性关节（可选）
  Eigen::MatrixXf global_rot_offsets_;   ///< [J, 9] 标准 T-pose 全局旋转偏移（可选）
  Eigen::MatrixXf rest_pose_local_rot_;  ///< [J, 9] 静止姿态局部旋转（可选，如 G1 骨骼 XML rest pose）

  // ======================================================================
  // 语义标签
  // ======================================================================

  std::vector<std::int64_t> left_foot_joint_idx_;   ///< 左脚关节索引（链顺序）
  std::vector<std::int64_t> right_foot_joint_idx_;  ///< 右脚关节索引（链顺序）
  std::vector<std::int64_t> left_hand_joint_idx_;   ///< 左手关节索引（链顺序）
  std::vector<std::int64_t> right_hand_joint_idx_;  ///< 右手关节索引（链顺序）
  std::vector<std::int64_t> foot_joint_idx_;        ///< 左脚 + 右脚联合索引
  std::vector<std::int64_t> hip_joint_idx_;         ///< [right, left] 髋关节索引

  // ======================================================================
  // 层级深度分组（用于 FK 逐级更新，对应 Python compute_idx_levels）
  // ======================================================================

  std::vector<std::vector<std::int64_t>> joint_levels_;

  // ======================================================================
  // 特殊数据（SOMASkeleton77 的放松手部姿态）
  // ======================================================================

  Eigen::MatrixXf relaxed_hands_rest_pose_;  ///< [J77, 9] 放松手部姿态局部旋转（仅 SOMA77）

  // ======================================================================
  // 构造 / 初始化
  // ======================================================================

  skeleton_base() = default;

  /// @brief 从关节层级列表初始化（由具体骨骼工厂调用）
  /// @param bone_hierarchy { (关节名, 父关节名), ... }，父关节为 nullptr/空 表示根关节
  void init_from_bone_hierarchy(const std::vector<std::pair<std::string, std::string>>& bone_hierarchy);

  // ======================================================================
  // npy 数据加载
  // ======================================================================

  /// @brief 从文件夹加载 neutral_joints (joints.npy)
  void load_neutral_joints(const FSys::path& folder);

  /// @brief 从文件夹加载 bvh_neutral_joints (bvh_joints.npy，可选)
  void load_bvh_neutral_joints(const FSys::path& folder);

  /// @brief 从文件夹加载 global_rot_offsets (standard_t_pose_global_offsets_rots.npy，可选)
  void load_global_rot_offsets(const FSys::path& folder);

  /// @brief 从文件夹加载 rest_pose_local_rot (rest_pose_local_rot.npy，可选)
  void load_rest_pose_local_rot(const FSys::path& folder);

  /// @brief 从文件夹加载 relaxed_hands_rest_pose (relaxed_hands_rest_pose.npy，可选)
  void load_relaxed_hands_rest_pose(const FSys::path& folder);

  /// @brief 从文件夹加载所有可用数据文件（不报错若文件不存在）
  void load_all_from_folder(const FSys::path& folder);

  // ======================================================================
  // 检查
  // ======================================================================

  [[nodiscard]] bool is_valid() const { return nbjoints_ > 0 && neutral_joints_.size() > 0; }

  // ======================================================================
  // 工厂函数：具体骨骼（返回共享指针）
  // ======================================================================

  /// @brief 创建 SOMA 30 关节骨骼
  static std::shared_ptr<skeleton_base> create_soma_skeleton_30(const FSys::path& folder = {});

  /// @brief 创建 SOMA 77 关节骨骼
  static std::shared_ptr<skeleton_base> create_soma_skeleton_77(const FSys::path& folder = {});

  /// @brief 创建 G1 34 关节骨骼
  static std::shared_ptr<skeleton_base> create_g1_skeleton_34(const FSys::path& folder = {});

  /// @brief 创建 SMPL-X 22 关节骨骼
  static std::shared_ptr<skeleton_base> create_smplx_skeleton_22(const FSys::path& folder = {});
};

}  // namespace doodle::ai
