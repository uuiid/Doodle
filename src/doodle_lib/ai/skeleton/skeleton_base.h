//
// Created by TD on 25-7-22.
//
#pragma once

#include <doodle_lib/ai/fwd.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <Eigen/Dense>
#include <cstdint>
#include <memory>
#include <optional>
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

  MatrixXfRow neutral_joints_;       ///< [J, 3] 中性姿态关节位置
  MatrixXfRow bvh_neutral_joints_;   ///< [J, 3] BVH 中性关节（可选）
  MatrixXfRow global_rot_offsets_;   ///< [J, 9] 标准 T-pose 全局旋转偏移（可选）
  MatrixXfRow rest_pose_local_rot_;  ///< [J, 9] 静止姿态局部旋转（可选，如 G1 骨骼 XML rest pose）

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

  MatrixXfRow relaxed_hands_rest_pose_;  ///< [J77, 9] 放松手部姿态局部旋转（仅 SOMA77）

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
  static std::shared_ptr<skeleton_base> create_soma_skeleton_30(
      const FSys::path& folder, const FSys::path& in_77_folder = {}
  );

  /// @brief 创建 SOMA 77 关节骨骼
  static std::shared_ptr<skeleton_base> create_soma_skeleton_77(const FSys::path& folder = {});

  // ======================================================================
  // 前向运动学 (FK)
  // ======================================================================

  /// @brief 前向运动学结果
  struct fk_result {
    MatrixXfRow global_rot_mats;         ///< [B*T, J*9] 全局旋转矩阵
    MatrixXfRow posed_joints;            ///< [B*T, J*3] 全局关节位置
    MatrixXfRow posed_joints_norootpos;  ///< [B*T, J*3] 无根偏移的关节位置
  };

  /// @brief 由局部旋转和根位置计算全局旋转和关节位置
  /// @param local_rot_mats [B*T, J*9] 局部旋转矩阵
  /// @param root_positions [B*T, 3] 根关节世界坐标
  /// @return 全局旋转、全局关节位置、无根偏移关节位置
  [[nodiscard]] fk_result fk(const MatrixXfRow& local_rot_mats, const MatrixXfRow& root_positions) const;

  /// @brief 由全局旋转矩阵计算局部旋转（逆父级变换）
  /// @param global_rot_mats [B*T, J*9] 全局旋转
  /// @return [B*T, J*9] 局部旋转
  [[nodiscard]] MatrixXfRow global_rots_to_local_rots(const MatrixXfRow& global_rot_mats) const;

  // ======================================================================
  // 骨骼间转换（SOMA30 ↔ SOMA77）
  // ======================================================================

  /// @brief 获取当前骨骼关节在目标骨骼中的索引映射。
  ///        对应当前骨骼 bone_order_names_ 中每个关节，返回其在 target.bone_index_ 中的索引。
  /// @param target 目标骨骼
  /// @return 索引列表，顺序对应当前骨骼的 bone_order_names_
  [[nodiscard]] std::vector<std::int64_t> get_skel_slice(const skeleton_base& target) const;

  /// @brief SOMA30 → SOMA77 转换结果
  struct output_77_result {
    MatrixXfRow local_rot_mats;                ///< [B*T, 77*9]
    MatrixXfRow global_rot_mats;               ///< [B*T, 77*9]
    MatrixXfRow posed_joints;                  ///< [B*T, 77*3]
    std::optional<MatrixXfRow> foot_contacts;  ///< [B*T, 6]（可选）
  };

  /// @brief 将 30 关节局部旋转扩展为 77 关节（含放松手部姿态）。
  ///        仅在骨骼为 somaskel30 时有效。
  /// @param local_joint_rots_subset [B*T, 30*9] 30 关节局部旋转矩阵
  /// @return [B*T, 77*9] 77 关节局部旋转矩阵
  [[nodiscard]] MatrixXfRow to_soma_skeleton_77(const MatrixXfRow& local_joint_rots_subset) const;

  /// @brief 将 SOMA30 模型输出转换为 SOMA77 格式。
  ///        扩展 local_rot_mats 至 77 关节，重新运行 FK 计算 global_rot_mats 和 posed_joints，
  ///        如有 foot_contacts 则从 4 通道扩展为 6 通道（toe-end 复制 toe-base）。
  /// @param local_rot_mats [B*T, 30*9] 30 关节局部旋转
  /// @param root_positions [B*T, 3] 根关节位置
  /// @param foot_contacts 可选 [B*T, 4]（L_heel, L_toe, R_heel, R_toe）
  /// @return SOMA77 格式的输出
  [[nodiscard]] output_77_result output_to_soma_skeleton_77(
      const MatrixXfRow& local_rot_mats, const MatrixXfRow& root_positions,
      const std::optional<MatrixXfRow>& foot_contacts = std::nullopt
  ) const;

 private:
  /// @brief SOMA77 骨骼缓存（用于 SOMA30 → SOMA77 转换时懒加载）
  mutable std::shared_ptr<skeleton_base> somaskel77_cache_;
};

}  // namespace doodle::ai
