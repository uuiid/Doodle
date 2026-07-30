//
// Created by TD on 25-7-21.
//
#pragma once
#include <doodle_lib/ai/fwd.h>
#include <doodle_lib/ai/motion_rep/motion_rep_base.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <Eigen/Dense>
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace doodle::ai {

struct kimodo_model_config;

// ======================================================================
// 运动解码输出（对应 Python KimodoMotionRep.inverse 的返回字典）
// ======================================================================

/// @brief KimodoMotionRep 解码输出结构
struct motion_output {
  MatrixXfRow local_rot_mats;       ///< [B*T, J*9] 局部旋转矩阵
  MatrixXfRow global_rot_mats;      ///< [B*T, J*9] 全局旋转矩阵
  MatrixXfRow posed_joints;         ///< [B*T, J*3] 全局关节位置
  MatrixXfRow root_positions;       ///< [B*T, 3] 根节点位置
  MatrixXfRow smooth_root_pos;      ///< [B*T, 3] 平滑根位置
  MatrixXbRow foot_contacts;        ///< [B*T, 4] 脚接触标签(不序列化)
  MatrixXfRow global_root_heading;  ///< [B*T, 2] 全局根朝向 (cos, sin)

  [[nodiscard]] bool is_valid() const { return local_rot_mats.size() > 0; }
  // to json
  // @note 输出时, 所有的 Row 为 T, B已经被拆分为多个 motion_output, 也就是每个 motion_output 对应一个 batch 元素
  // 同时, J*9 要转换为 J*3*3, 也就是每个关节的旋转矩阵
  friend void to_json(nlohmann::json& j, const motion_output& p) {
    p.save_matrix_to_json_3_3(j["local_rot_mats"], p.local_rot_mats);
    p.save_matrix_to_json_3_3(j["global_rot_mats"], p.global_rot_mats);
    p.save_matrix_to_json_3(j["posed_joints"], p.posed_joints);
    p.save_matrix_to_json(j["root_positions"], p.root_positions);
    p.save_matrix_to_json(j["smooth_root_pos"], p.smooth_root_pos);
    // p.save_matrix_to_json(j, p.foot_contacts);
    p.save_matrix_to_json(j["global_root_heading"], p.global_root_heading);
  }

 private:
  void save_matrix_to_json_3_3(nlohmann::json& j, const MatrixXfRow& mat) const {
    // mat size: [N, J * 9], need to convert to [N, J, 3, 3]
    using mat_33 = std::array<std::array<float, 3>, 3>;
    DOODLE_CHICK(mat.cols() % 9 == 0, "mat 列数不是 9 的倍数: {}", mat.cols());
    const Eigen::Index rows = mat.rows();
    for (auto i = 0; i < rows; ++i) {
      const auto row       = mat.row(i);
      const Eigen::Index J = row.size() / 9;
      std::vector<mat_33> joints(J);
      for (auto j = 0; j < J; ++j) {
        const auto joint_row = row.segment(j * 9, 9);
        mat_33 joint_mat{};
        for (auto r = 0; r < 3; ++r) {
          for (auto c = 0; c < 3; ++c) {
            joint_mat[r][c] = joint_row(r * 3 + c);
          }
        }
        joints[j] = joint_mat;
      }
      j.push_back(joints);
    }
  }
  void save_matrix_to_json_3(nlohmann::json& j, const MatrixXfRow& mat) const {
    // mat size: [N, J * 3], need to convert to [N, J, 3]
    using mat_3 = std::array<float, 3>;
    DOODLE_CHICK(mat.cols() % 3 == 0, "mat 列数不是 3 的倍数: {}", mat.cols());
    const Eigen::Index rows = mat.rows();
    for (auto i = 0; i < rows; ++i) {
      const auto row       = mat.row(i);
      const Eigen::Index J = row.size() / 3;
      std::vector<mat_3> joints(J);
      for (auto j = 0; j < J; ++j) {
        const auto joint_row = row.segment(j * 3, 3);
        mat_3 joint_vec{};
        for (auto c = 0; c < 3; ++c) {
          joint_vec[c] = joint_row(c);
        }
        joints[j] = joint_vec;
      }
      j.push_back(joints);
    }
  }
  void save_matrix_to_json(nlohmann::json& j, const MatrixXfRow& mat) const {
    // mat size: [N, D], need to convert to [N, D]
    const Eigen::Index rows = mat.rows();
    for (auto i = 0; i < rows; ++i) {
      const auto row = mat.row(i);
      std::vector<float> row_vec(row.data(), row.data() + row.size());
      j.push_back(row_vec);
    }
  }
};

// ======================================================================
// Kimodo 运动表示（对应 Python KimodoMotionRep）
// ======================================================================

/// @brief Kimodo 平滑根运动表示
///
/// 特征布局（SOMA30 示例, nbjoints=30）:
///   smooth_root_pos:        3
///   global_root_heading:    2
///   local_joints_positions: J*3
///   global_rot_data:        J*6
///   velocities:             J*3
///   foot_contacts:          4
///   TOTAL:                  369 (for J=30)
///
/// root_slice = [0, 5)  = smooth_root_pos + global_root_heading
/// body_slice = [5, D)  = local_joints_positions + global_rot_data + velocities + foot_contacts
class kimodo_motion_rep : public motion_rep_base {
 public:
  /// @brief 构造函数
  /// @param skel 骨骼定义（共享指针）
  /// @param cfg 模型配置（共享指针），提供 fps、stats_path 等参数
  kimodo_motion_rep(std::shared_ptr<skeleton_base> skel, std::shared_ptr<kimodo_model_config> cfg);

  // ======================================================================
  // 编码：局部旋转 + 根位置 → 平滑根特征（对应 Python __call__）
  // ======================================================================

  /// @brief 将局部旋转和根轨迹编码为平滑根特征
  /// @param local_joint_rots [B*T, J*9] 局部旋转矩阵
  /// @param root_positions [B*T, 3] 根位置
  /// @param to_normalize 是否标准化输出
  /// @param batch_size B
  /// @param time_steps T
  /// @param lengths [B] 各样本有效帧数（可选）
  /// @return [B*T, motion_rep_dim] 运动特征
  MatrixXfRow encode(
      const MatrixXfRow& local_joint_rots, const MatrixXfRow& root_positions, bool to_normalize,
      std::int64_t batch_size, std::int64_t time_steps, const Eigen::VectorXi& lengths = {}
  ) const;

  // ======================================================================
  // 解码：平滑根特征 → 运动输出（对应 Python inverse）
  // ======================================================================

  /// @brief 将平滑根特征解码为运动张量
  /// @param features [B*T, motion_rep_dim] 运动特征
  /// @param is_normalized 输入是否已标准化
  /// @param batch_size B
  /// @param time_steps T
  /// @return motion_output 结构
  motion_output decode(
      const MatrixXfRow& features, bool is_normalized, std::int64_t batch_size, std::int64_t time_steps
  ) const;

  // ======================================================================
  // 几何变换（重写基类虚函数）
  // ======================================================================

  /// @brief 旋转特征（对应 Python rotate）
  MatrixXfRow rotate(
      const MatrixXfRow& features, const Eigen::VectorXf& angle, std::int64_t batch_size, std::int64_t time_steps
  ) const override;

  /// @brief 平移 2D（对应 Python translate_2d）
  MatrixXfRow translate_2d(
      const MatrixXfRow& features, const MatrixXfRow& translation_2d, std::int64_t batch_size, std::int64_t time_steps
  ) const override;

  // ======================================================================
  // 条件创建（对应 Python create_conditions）
  // ======================================================================

  /// @brief 从约束构建条件和掩码
  /// 注意：这是简化的单序列版本，返回 [T, D] 大小的矩阵。
  /// 上层调用方需要自行 repeat 为 [B, T, D]。
  ///
  /// @param index_dict 索引字典（约束构建的输出）
  /// @param data_dict 数据字典（约束构建的输出）
  /// @param length 序列长度
  /// @param to_normalize 是否标准化
  /// @return (observed_motion [T, D], motion_mask [T, D])
  struct condition_result {
    MatrixXfRow observed_motion;                                      ///< [T, D] 观测运动
    Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic> motion_mask;  ///< [T, D] 运动掩码
  };
  condition_result create_conditions(
      const std::unordered_map<std::string, std::vector<MatrixXfRow>>& index_dict,
      const std::unordered_map<std::string, std::vector<MatrixXfRow>>& data_dict, std::int64_t length, bool to_normalize
  ) const;

  /// @brief 批量创建条件（对应 Python create_conditions_from_constraints_batched）
  /// @param constraints_lst 每个样本的约束列表
  /// @param lengths [B] 各样本长度
  /// @param to_normalize 是否标准化
  /// @return (observed_motion [B*T, D], motion_mask [B*T, D])
  struct batched_condition_result {
    MatrixXfRow observed_motion;                                      ///< [B*T, D]
    Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic> motion_mask;  ///< [B*T, D]
  };
  batched_condition_result create_conditions_from_constraints_batched(
      const std::vector<std::vector<std::pair<std::string, std::vector<MatrixXfRow>>>>& constraints_lst,
      const Eigen::VectorXi& lengths, bool to_normalize
  ) const;
};

}  // namespace doodle::ai
