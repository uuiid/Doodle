//
// Created by TD on 25-7-28.
//
#pragma once

#include <doodle_lib/ai/skeleton/skeleton_base.h>
#include <doodle_lib/core/global_function.h>

#include <Eigen/Dense>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace doodle::ai {

// ======================================================================
// 前向声明
// ======================================================================

class skeleton_base;

// ======================================================================
// 工具函数：将 JSON 数组转换为 Eigen 矩阵
// ======================================================================

/// @brief 将嵌套 JSON 数组（1D/2D/3D）转换为 Eigen 矩阵
/// @tparam T 标量类型（float / std::int64_t）
/// @param j JSON 数据
/// @return 行优先的 Eigen 矩阵
/// @note
///   - 一维数组 → [N, 1] 列向量
///   - 二维数组 → [M, N] 矩阵
///   - 三维数组 [M, N, K] → flatten 为 [M*N, K] 矩阵
template <typename T = float>
Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> json_to_eigen_matrix(const nlohmann::json& j) {
  if (!j.is_array() || j.empty()) return {};

  // 检查嵌套深度: j[0][0] 是否为数组 → 3D
  const bool is_3d = j[0].is_array() && !j[0].empty() && j[0][0].is_array();

  if (is_3d) {
    // 三维数组: [[[...], ...], ...] — 形状 [M, N, K]
    const Eigen::Index M = static_cast<Eigen::Index>(j.size());
    const Eigen::Index N = static_cast<Eigen::Index>(j[0].size());
    const Eigen::Index K = static_cast<Eigen::Index>(j[0][0].size());
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> mat(M * N, K);
    for (Eigen::Index m = 0; m < M; ++m) {
      const auto& jm = j[static_cast<std::size_t>(m)];
      for (Eigen::Index n = 0; n < N; ++n) {
        const auto& jmn = jm[static_cast<std::size_t>(n)];
        for (Eigen::Index k = 0; k < K; ++k) {
          mat(m * N + n, k) = static_cast<T>(jmn[static_cast<std::size_t>(k)]);
        }
      }
    }
    return mat;
  }

  if (j[0].is_array()) {
    // 二维数组: [[...], [...], ...]
    const Eigen::Index rows = static_cast<Eigen::Index>(j.size());
    const Eigen::Index cols = static_cast<Eigen::Index>(j[0].size());
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> mat(rows, cols);
    for (Eigen::Index r = 0; r < rows; ++r) {
      const auto& row = j[static_cast<std::size_t>(r)];
      for (Eigen::Index c = 0; c < cols; ++c) {
        mat(r, c) = static_cast<T>(row[static_cast<std::size_t>(c)]);
      }
    }
    return mat;
  }

  // 一维数组: [v0, v1, ...]
  const Eigen::Index n = static_cast<Eigen::Index>(j.size());
  Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> mat(n, 1);
  for (Eigen::Index i = 0; i < n; ++i) {
    mat(i, 0) = static_cast<T>(j[static_cast<std::size_t>(i)]);
  }
  return mat;
}

/// @brief 将 Eigen 矩阵转换为 JSON 数组（二维数组格式）
template <typename Derived>
nlohmann::json eigen_matrix_to_json(const Eigen::DenseBase<Derived>& mat) {
  nlohmann::json j        = nlohmann::json::array();
  const Eigen::Index rows = mat.rows();
  const Eigen::Index cols = mat.cols();
  for (Eigen::Index r = 0; r < rows; ++r) {
    nlohmann::json row = nlohmann::json::array();
    for (Eigen::Index c = 0; c < cols; ++c) {
      row.push_back(static_cast<double>(mat(r, c)));
    }
    j.push_back(std::move(row));
  }
  return j;
}

// ======================================================================
// Root2DConstraintSet — 根轨迹约束
// ======================================================================

/// @brief 根轨迹约束（对应 Python Root2DConstraintSet）
/// 固定根节点 (x, z) 轨迹和可选全局朝向。
class root2d_constraint_set {
 public:
  static constexpr const char* name = "root2d";

  std::shared_ptr<skeleton_base> skeleton_;
  Eigen::VectorXi frame_indices_;        ///< [K] 约束帧索引
  Eigen::MatrixXf smooth_root_2d_;       ///< [K, 2] 平滑根位置 (x, z)
  Eigen::MatrixXf global_root_heading_;  ///< [K, 2] 可选全局朝向 (cos, sin)，为空表示不约束

  root2d_constraint_set() = default;

  /// @brief 构造
  /// @param skeleton 骨骼定义
  /// @param frame_indices [K] 帧索引
  /// @param smooth_root_2d [K, 2] 平滑根 (x, z)
  /// @param global_root_heading [K, 2] 可选全局朝向
  root2d_constraint_set(
      std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, Eigen::MatrixXf smooth_root_2d,
      Eigen::MatrixXf global_root_heading = {}
  );

  // ======================================================================
  // 接口
  // ======================================================================

  /// @brief 将约束数据追加到 data_dict / index_dict（供 create_conditions 使用）
  void update_constraints(
      std::unordered_map<std::string, std::vector<Eigen::MatrixXf>>& data_dict,
      std::unordered_map<std::string, std::vector<Eigen::VectorXi>>& index_dict
  ) const;

  /// @brief 裁剪到 [start, end) 帧范围，返回新对象
  root2d_constraint_set crop_move(std::int64_t start, std::int64_t end) const;

  /// @brief 移动数据
  void to(const std::shared_ptr<skeleton_base>& skel = {});

  /// @brief 从 JSON 字典反序列化
  static root2d_constraint_set from_dict(std::shared_ptr<skeleton_base> skeleton, const nlohmann::json& dico);
};

// ======================================================================
// FullBodyConstraintSet — 全身约束
// ======================================================================

/// @brief 全身约束（对应 Python FullBodyConstraintSet）
/// 固定所有关节的全局位置和旋转。
class fullbody_constraint_set {
 public:
  static constexpr const char* name = "fullbody";

  std::shared_ptr<skeleton_base> skeleton_;
  Eigen::VectorXi frame_indices_;            ///< [K] 约束帧索引
  Eigen::MatrixXf global_joints_positions_;  ///< [K, J*3] 全局关节位置
  Eigen::MatrixXf global_joints_rots_;       ///< [K, J*9] 全局旋转矩阵
  Eigen::VectorXf root_y_pos_;               ///< [K] 根关节 Y 位置
  Eigen::MatrixXf global_root_heading_;      ///< [K, 2] 全局朝向 (cos, sin)
  Eigen::MatrixXf smooth_root_2d_;           ///< [K, 2] 平滑根位置 (x, z)

  fullbody_constraint_set() = default;

  fullbody_constraint_set(
      std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, Eigen::MatrixXf global_joints_positions,
      Eigen::MatrixXf global_joints_rots, Eigen::MatrixXf smooth_root_2d = {}
  );

  void update_constraints(
      std::unordered_map<std::string, std::vector<Eigen::MatrixXf>>& data_dict,
      std::unordered_map<std::string, std::vector<Eigen::VectorXi>>& index_dict
  ) const;

  fullbody_constraint_set crop_move(std::int64_t start, std::int64_t end) const;

  void to(const std::shared_ptr<skeleton_base>& skel = {});

  static fullbody_constraint_set from_dict(std::shared_ptr<skeleton_base> skeleton, const nlohmann::json& dico);
};

// ======================================================================
// EndEffectorConstraintSet — 末端执行器约束
// ======================================================================

/// @brief 末端执行器约束（对应 Python EndEffectorConstraintSet）
/// 固定指定关节（如手/脚）的全局位置和旋转。
class end_effector_constraint_set {
 public:
  static constexpr const char* name = "end-effector";

  std::shared_ptr<skeleton_base> skeleton_;
  Eigen::VectorXi frame_indices_;            ///< [K] 约束帧索引
  std::vector<std::string> joint_names_;     ///< 关节名称列表
  Eigen::VectorXi pos_indices_;              ///< 位置索引（对应 skeleton 关节索引）
  Eigen::VectorXi rot_indices_;              ///< 旋转索引（对应 skeleton 关节索引）
  Eigen::MatrixXf global_joints_positions_;  ///< [K, selected_pos*3] 选中关节位置
  Eigen::MatrixXf global_joints_rots_;       ///< [K, selected_rot*9] 选中关节旋转
  Eigen::VectorXf root_y_pos_;               ///< [K] 根关节 Y 位置
  Eigen::MatrixXf global_root_heading_;      ///< [K, 2] 全局朝向
  Eigen::MatrixXf smooth_root_2d_;           ///< [K, 2] 平滑根位置

  end_effector_constraint_set() = default;

  end_effector_constraint_set(
      std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, Eigen::MatrixXf global_joints_positions,
      Eigen::MatrixXf global_joints_rots, Eigen::MatrixXf smooth_root_2d, std::vector<std::string> joint_names
  );

  void update_constraints(
      std::unordered_map<std::string, std::vector<Eigen::MatrixXf>>& data_dict,
      std::unordered_map<std::string, std::vector<Eigen::VectorXi>>& index_dict
  ) const;

  end_effector_constraint_set crop_move(std::int64_t start, std::int64_t end) const;

  void to(const std::shared_ptr<skeleton_base>& skel = {});

  static end_effector_constraint_set from_dict(std::shared_ptr<skeleton_base> skeleton, const nlohmann::json& dico);
};

// ======================================================================
// 专用末端执行器约束子类
// ======================================================================

/// @brief 左手约束
class left_hand_constraint_set : public end_effector_constraint_set {
 public:
  static constexpr const char* name = "left-hand";
  left_hand_constraint_set() { joint_names_ = {"LeftHand"}; }
  left_hand_constraint_set(
      std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, Eigen::MatrixXf global_joints_positions,
      Eigen::MatrixXf global_joints_rots, Eigen::MatrixXf smooth_root_2d
  )
      : end_effector_constraint_set(
            std::move(skeleton), std::move(frame_indices), std::move(global_joints_positions),
            std::move(global_joints_rots), std::move(smooth_root_2d), {"LeftHand"}
        ) {}
};

/// @brief 右手约束
class right_hand_constraint_set : public end_effector_constraint_set {
 public:
  static constexpr const char* name = "right-hand";
  right_hand_constraint_set() { joint_names_ = {"RightHand"}; }
  right_hand_constraint_set(
      std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, Eigen::MatrixXf global_joints_positions,
      Eigen::MatrixXf global_joints_rots, Eigen::MatrixXf smooth_root_2d
  )
      : end_effector_constraint_set(
            std::move(skeleton), std::move(frame_indices), std::move(global_joints_positions),
            std::move(global_joints_rots), std::move(smooth_root_2d), {"RightHand"}
        ) {}
};

/// @brief 左脚约束
class left_foot_constraint_set : public end_effector_constraint_set {
 public:
  static constexpr const char* name = "left-foot";
  left_foot_constraint_set() { joint_names_ = {"LeftFoot"}; }
  left_foot_constraint_set(
      std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, Eigen::MatrixXf global_joints_positions,
      Eigen::MatrixXf global_joints_rots, Eigen::MatrixXf smooth_root_2d
  )
      : end_effector_constraint_set(
            std::move(skeleton), std::move(frame_indices), std::move(global_joints_positions),
            std::move(global_joints_rots), std::move(smooth_root_2d), {"LeftFoot"}
        ) {}
};

/// @brief 右脚约束
class right_foot_constraint_set : public end_effector_constraint_set {
 public:
  static constexpr const char* name = "right-foot";
  right_foot_constraint_set() { joint_names_ = {"RightFoot"}; }
  right_foot_constraint_set(
      std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, Eigen::MatrixXf global_joints_positions,
      Eigen::MatrixXf global_joints_rots, Eigen::MatrixXf smooth_root_2d
  )
      : end_effector_constraint_set(
            std::move(skeleton), std::move(frame_indices), std::move(global_joints_positions),
            std::move(global_joints_rots), std::move(smooth_root_2d), {"RightFoot"}
        ) {}
};

// ======================================================================
// 约束集合类型擦除包装
// ======================================================================

/// @brief 所有约束集合类型的变体
using constraint_set_var = std::variant<
    root2d_constraint_set, fullbody_constraint_set, end_effector_constraint_set, left_hand_constraint_set,
    right_hand_constraint_set, left_foot_constraint_set, right_foot_constraint_set>;

// ======================================================================
// 加载 / 保存约束列表
// ======================================================================

/// @brief 从 JSON 路径或数据加载约束列表（对应 Python load_constraints_lst）
/// @param path_or_data JSON 文件路径
/// @param skeleton 骨骼定义
/// @return 约束列表
std::vector<constraint_set_var> load_constraints_lst(const FSys::path& path, std::shared_ptr<skeleton_base> skeleton);

/// @brief 从 JSON 数据加载约束列表（对应 Python load_constraints_lst）
/// @param json_data nlohmann::json 数组
/// @param skeleton 骨骼定义
/// @return 约束列表
std::vector<constraint_set_var> load_constraints_lst_from_json(
    const nlohmann::json& json_data, std::shared_ptr<skeleton_base> skeleton
);

// ======================================================================
// 辅助：约束类型名称获取
// ======================================================================

/// @brief 获取约束变体的类型名称
std::string get_constraint_type_name(const constraint_set_var& constraint);

}  // namespace doodle::ai
