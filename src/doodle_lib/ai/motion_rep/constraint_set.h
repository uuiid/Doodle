//
// Created by TD on 25-7-28.
//
#pragma once

#include <doodle_lib/ai/fwd.h>
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
// ConstraintSetBase — 约束集纯虚基类
// ======================================================================

/// @brief 所有约束集类型的纯虚基类
class constraint_set_base {
 public:
  virtual ~constraint_set_base() = default;

  /// @brief 返回约束类型名称
  virtual std::string type_name() const = 0;

  /// @brief 将约束数据追加到 data_dict / index_dict（供 create_conditions 使用）
  virtual void update_constraints(
      std::unordered_map<std::string, std::vector<MatrixXfRow>>& data_dict,
      std::unordered_map<std::string, std::vector<Eigen::VectorXi>>& index_dict
  ) const = 0;

  /// @brief 裁剪到 [start, end) 帧范围，返回新对象
  virtual std::shared_ptr<constraint_set_base> crop_move(std::int64_t start, std::int64_t end) const = 0;

  /// @brief 设置骨骼
  virtual void to(const std::shared_ptr<skeleton_base>& skel) = 0;

  /// @brief 获取骨骼
  virtual std::shared_ptr<skeleton_base> get_skeleton() const = 0;
};

// ======================================================================
// Root2DConstraintSet — 根轨迹约束
// ======================================================================

/// @brief 根轨迹约束（对应 Python Root2DConstraintSet）
/// 固定根节点 (x, z) 轨迹和可选全局朝向。
class root2d_constraint_set : public constraint_set_base {
 public:
  static constexpr const char* name = "root2d";

  std::shared_ptr<skeleton_base> skeleton_;
  Eigen::VectorXi frame_indices_;    ///< [K] 约束帧索引
  MatrixXfRow smooth_root_2d_;       ///< [K, 2] 平滑根位置 (x, z)
  MatrixXfRow global_root_heading_;  ///< [K, 2] 可选全局朝向 (cos, sin)，为空表示不约束

  root2d_constraint_set() = default;

  /// @brief 构造
  /// @param skeleton 骨骼定义
  /// @param frame_indices [K] 帧索引
  /// @param smooth_root_2d [K, 2] 平滑根 (x, z)
  /// @param global_root_heading [K, 2] 可选全局朝向
  root2d_constraint_set(
      std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, MatrixXfRow smooth_root_2d,
      MatrixXfRow global_root_heading = {}
  );

  // ======================================================================
  // 接口
  // ======================================================================

  std::string type_name() const override { return name; }

  void update_constraints(
      std::unordered_map<std::string, std::vector<MatrixXfRow>>& data_dict,
      std::unordered_map<std::string, std::vector<Eigen::VectorXi>>& index_dict
  ) const override;

  std::shared_ptr<constraint_set_base> crop_move(std::int64_t start, std::int64_t end) const override;

  void to(const std::shared_ptr<skeleton_base>& skel) override;

  std::shared_ptr<skeleton_base> get_skeleton() const override { return skeleton_; }

  /// @brief 从 JSON 字典反序列化
  static std::shared_ptr<root2d_constraint_set> from_dict(
      std::shared_ptr<skeleton_base> skeleton, const nlohmann::json& dico
  );
};

// ======================================================================
// FullBodyConstraintSet — 全身约束
// ======================================================================

/// @brief 全身约束（对应 Python FullBodyConstraintSet）
/// 固定所有关节的全局位置和旋转。
class fullbody_constraint_set : public constraint_set_base {
 public:
  static constexpr const char* name = "fullbody";

  std::shared_ptr<skeleton_base> skeleton_;
  Eigen::VectorXi frame_indices_;        ///< [K] 约束帧索引
  MatrixXfRow global_joints_positions_;  ///< [K, J*3] 全局关节位置
  MatrixXfRow global_joints_rots_;       ///< [K, J*9] 全局旋转矩阵
  Eigen::VectorXf root_y_pos_;           ///< [K] 根关节 Y 位置
  MatrixXfRow global_root_heading_;      ///< [K, 2] 全局朝向 (cos, sin)
  MatrixXfRow smooth_root_2d_;           ///< [K, 2] 平滑根位置 (x, z)

  fullbody_constraint_set() = default;

  fullbody_constraint_set(
      std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, MatrixXfRow global_joints_positions,
      MatrixXfRow global_joints_rots, MatrixXfRow smooth_root_2d = {}
  );

  std::string type_name() const override { return name; }

  void update_constraints(
      std::unordered_map<std::string, std::vector<MatrixXfRow>>& data_dict,
      std::unordered_map<std::string, std::vector<Eigen::VectorXi>>& index_dict
  ) const override;

  std::shared_ptr<constraint_set_base> crop_move(std::int64_t start, std::int64_t end) const override;

  void to(const std::shared_ptr<skeleton_base>& skel) override;

  std::shared_ptr<skeleton_base> get_skeleton() const override { return skeleton_; }

  static std::shared_ptr<fullbody_constraint_set> from_dict(
      std::shared_ptr<skeleton_base> skeleton, const nlohmann::json& dico
  );
};

// ======================================================================
// EndEffectorConstraintSet — 末端执行器约束
// ======================================================================

/// @brief 末端执行器约束（对应 Python EndEffectorConstraintSet）
/// 固定指定关节（如手/脚）的全局位置和旋转。
class end_effector_constraint_set : public constraint_set_base {
 public:
  static constexpr const char* name = "end-effector";

  std::shared_ptr<skeleton_base> skeleton_;
  Eigen::VectorXi frame_indices_;         ///< [K] 约束帧索引
  std::vector<std::string> joint_names_;  ///< 关节名称列表
  Eigen::VectorXi pos_indices_;           ///< 位置索引（对应 skeleton 关节索引）
  Eigen::VectorXi rot_indices_;           ///< 旋转索引（对应 skeleton 关节索引）
  MatrixXfRow global_joints_positions_;   ///< [K, selected_pos*3] 选中关节位置
  MatrixXfRow global_joints_rots_;        ///< [K, selected_rot*9] 选中关节旋转
  Eigen::VectorXf root_y_pos_;            ///< [K] 根关节 Y 位置
  MatrixXfRow global_root_heading_;       ///< [K, 2] 全局朝向
  MatrixXfRow smooth_root_2d_;            ///< [K, 2] 平滑根位置

  end_effector_constraint_set() = default;

  end_effector_constraint_set(
      std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, MatrixXfRow global_joints_positions,
      MatrixXfRow global_joints_rots, MatrixXfRow smooth_root_2d, std::vector<std::string> joint_names
  );

  std::string type_name() const override { return name; }

  void update_constraints(
      std::unordered_map<std::string, std::vector<MatrixXfRow>>& data_dict,
      std::unordered_map<std::string, std::vector<Eigen::VectorXi>>& index_dict
  ) const override;

  std::shared_ptr<constraint_set_base> crop_move(std::int64_t start, std::int64_t end) const override;

  void to(const std::shared_ptr<skeleton_base>& skel) override;

  std::shared_ptr<skeleton_base> get_skeleton() const override { return skeleton_; }

  static std::shared_ptr<end_effector_constraint_set> from_dict(
      std::shared_ptr<skeleton_base> skeleton, const nlohmann::json& dico
  );
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
      std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, MatrixXfRow global_joints_positions,
      MatrixXfRow global_joints_rots, MatrixXfRow smooth_root_2d
  )
      : end_effector_constraint_set(
            std::move(skeleton), std::move(frame_indices), std::move(global_joints_positions),
            std::move(global_joints_rots), std::move(smooth_root_2d), {"LeftHand"}
        ) {}
  std::string type_name() const override { return name; }
};

/// @brief 右手约束
class right_hand_constraint_set : public end_effector_constraint_set {
 public:
  static constexpr const char* name = "right-hand";
  right_hand_constraint_set() { joint_names_ = {"RightHand"}; }
  right_hand_constraint_set(
      std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, MatrixXfRow global_joints_positions,
      MatrixXfRow global_joints_rots, MatrixXfRow smooth_root_2d
  )
      : end_effector_constraint_set(
            std::move(skeleton), std::move(frame_indices), std::move(global_joints_positions),
            std::move(global_joints_rots), std::move(smooth_root_2d), {"RightHand"}
        ) {}
  std::string type_name() const override { return name; }
};

/// @brief 左脚约束
class left_foot_constraint_set : public end_effector_constraint_set {
 public:
  static constexpr const char* name = "left-foot";
  left_foot_constraint_set() { joint_names_ = {"LeftFoot"}; }
  left_foot_constraint_set(
      std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, MatrixXfRow global_joints_positions,
      MatrixXfRow global_joints_rots, MatrixXfRow smooth_root_2d
  )
      : end_effector_constraint_set(
            std::move(skeleton), std::move(frame_indices), std::move(global_joints_positions),
            std::move(global_joints_rots), std::move(smooth_root_2d), {"LeftFoot"}
        ) {}
  std::string type_name() const override { return name; }
};

/// @brief 右脚约束
class right_foot_constraint_set : public end_effector_constraint_set {
 public:
  static constexpr const char* name = "right-foot";
  right_foot_constraint_set() { joint_names_ = {"RightFoot"}; }
  right_foot_constraint_set(
      std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, MatrixXfRow global_joints_positions,
      MatrixXfRow global_joints_rots, MatrixXfRow smooth_root_2d
  )
      : end_effector_constraint_set(
            std::move(skeleton), std::move(frame_indices), std::move(global_joints_positions),
            std::move(global_joints_rots), std::move(smooth_root_2d), {"RightFoot"}
        ) {}
  std::string type_name() const override { return name; }
};

// ======================================================================
// 约束集合类型擦除包装
// ======================================================================

/// @brief 所有约束集合类型的变体（保留兼容）
using constraint_set_var = std::variant<
    root2d_constraint_set, fullbody_constraint_set, end_effector_constraint_set, left_hand_constraint_set,
    right_hand_constraint_set, left_foot_constraint_set, right_foot_constraint_set>;

/// @brief 约束集基类共享指针
using constraint_set_ptr = std::shared_ptr<constraint_set_base>;

// ======================================================================
// 加载 / 保存约束列表
// ======================================================================

/// @brief 从 JSON 路径加载约束列表
/// @param path JSON 文件路径
/// @param skeleton 骨骼定义
/// @return 约束列表（基类共享指针）
std::vector<constraint_set_ptr> load_constraints_lst(
    const FSys::path& path, std::shared_ptr<skeleton_base> skeleton
);

/// @brief 从 JSON 数据加载约束列表
/// @param json_data nlohmann::json 数组
/// @param skeleton 骨骼定义
/// @return 约束列表（基类共享指针）
std::vector<constraint_set_ptr> load_constraints_lst_from_json(
    const nlohmann::json& json_data, std::shared_ptr<skeleton_base> skeleton
);

// ======================================================================
// 辅助：约束类型名称获取
// ======================================================================

/// @brief 获取约束变体的类型名称
std::string get_constraint_type_name(const constraint_set_var& constraint);

}  // namespace doodle::ai
