//
// Created by TD on 25-7-28.
//
#include "constraint_set.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/ai/motion_rep/feature_utils.h>
#include <doodle_lib/ai/motion_rep/geometry.h>

#include <boost/numeric/conversion/cast.hpp>

#include <Eigen/Core>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace doodle::ai {

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

// 固定尺寸矩阵特化: 支持 2D 和 3D JSON 数组
// 3D 数组 [M, N, K] → flatten 为 [M*N, K] 矩阵
template <typename T, std::int32_t Rows, std::int32_t Cols>
Eigen::Matrix<T, Rows, Cols> json_to_eigen_matrix(const nlohmann::json& j) {
  DOODLE_CHICK(j.is_array(), "json_to_eigen_matrix<{}, {}>: 期望 JSON 数组", Rows, Cols);
  DOODLE_CHICK(!j.empty(), "json_to_eigen_matrix<{}, {}>: 数组为空", Rows, Cols);

  const bool is_3d = j[0].is_array() && !j[0].empty() && j[0][0].is_array();

  if (is_3d) {
    // 三维数组: [[[...], ...], ...] — 形状 [M, N, K] → flatten [M*N, K]
    const Eigen::Index M = static_cast<Eigen::Index>(j.size());
    const Eigen::Index N = static_cast<Eigen::Index>(j[0].size());
    const Eigen::Index K = static_cast<Eigen::Index>(j[0][0].size());
    if constexpr (Rows != Eigen::Dynamic) {
      DOODLE_CHICK(M * N == Rows, "json_to_eigen_matrix<{}, {}>: flatten 行数 {} != 期望 {}", Rows, Cols, M * N, Rows);
    }
    if constexpr (Cols != Eigen::Dynamic) {
      DOODLE_CHICK(K == Cols, "json_to_eigen_matrix<{}, {}>: 最后一维 {} != 期望 {}", Rows, Cols, K, Cols);
    }
    Eigen::Matrix<T, Rows, Cols> mat(M * N, K);
    for (Eigen::Index m = 0; m < M; ++m) {
      const auto& jm = j[static_cast<std::size_t>(m)];
      for (Eigen::Index n = 0; n < N; ++n) {
        const auto& jmn = jm[static_cast<std::size_t>(n)];
        for (Eigen::Index k = 0; k < K; ++k) {
          mat(m * N + n, k) = jmn[static_cast<std::size_t>(k)].get<T>();
        }
      }
    }
    return mat;
  }

  // 二维数组: [[...], [...], ...]
  DOODLE_CHICK(j[0].is_array(), "json_to_eigen_matrix<{}, {}>: 期望二维数组，实际为一维", Rows, Cols);

  const Eigen::Index rows = static_cast<Eigen::Index>(j.size());
  const Eigen::Index cols = static_cast<Eigen::Index>(j[0].size());
  if constexpr (Rows != Eigen::Dynamic) {
    DOODLE_CHICK(rows == Rows, "json_to_eigen_matrix<{}, {}>: 行数 {} != 期望 {}", Rows, Cols, rows, Rows);
  }
  if constexpr (Cols != Eigen::Dynamic) {
    DOODLE_CHICK(cols == Cols, "json_to_eigen_matrix<{}, {}>: 列数 {} != 期望 {}", Rows, Cols, cols, Cols);
  }

  Eigen::Matrix<T, Rows, Cols> mat(rows, cols);
  for (Eigen::Index r = 0; r < rows; ++r) {
    const auto& row = j[static_cast<std::size_t>(r)];
    for (Eigen::Index c = 0; c < cols; ++c) {
      mat(r, c) = row[static_cast<std::size_t>(c)].get<T>();
    }
  }
  return mat;
}

// 固定尺寸列向量特化: Eigen::Matrix<T, Rows, 1>
template <typename T, std::int32_t Rows>
Eigen::Matrix<T, Rows, 1> json_to_eigen_matrix(const nlohmann::json& j) {
  DOODLE_CHICK(j.is_array(), "json_to_eigen_matrix<{}, 1>: 期望 JSON 数组", Rows);

  const Eigen::Index n = static_cast<Eigen::Index>(j.size());
  if constexpr (Rows != Eigen::Dynamic) {
    DOODLE_CHICK(n == Rows, "json_to_eigen_matrix<{}, 1>: 元素个数 {} != 期望 {}", Rows, n, Rows);
  }

  if (n == 0) return Eigen::Matrix<T, Rows, 1>{};

  DOODLE_CHICK(!j[0].is_array(), "json_to_eigen_matrix<{}, 1>: 期望一维数组，实际为多维", Rows);

  Eigen::Matrix<T, Rows, 1> mat(n);
  for (Eigen::Index i = 0; i < n; ++i) {
    mat(i, 0) = j[static_cast<std::size_t>(i)].get<T>();
  }
  return mat;
}

// ======================================================================
// 辅助函数：计算全局朝向 (cos, sin) 用于约束初始化
// ======================================================================
namespace detail {
MatrixX2fRow compute_global_heading_from_positions(
    const MatrixXfRow& global_joints_positions, const skeleton_base& skeleton
) {
  const Eigen::Index K            = global_joints_positions.rows();
  const MatrixXfRow heading_angle = compute_heading_angle(global_joints_positions, skeleton, 1, K);
  // heading_angle: [1, K]
  MatrixX2fRow heading(K, 2);
  for (Eigen::Index i = 0; i < K; ++i) {
    heading(i, 0) = std::cos(heading_angle(0, i));
    heading(i, 1) = std::sin(heading_angle(0, i));
  }
  return heading;
}

// 根据 JSON 中 type 字段构造正确的约束变体
std::shared_ptr<constraint_set_base> make_constraint_from_type(
    const std::string& type, std::shared_ptr<skeleton_base> skeleton, const nlohmann::json& dico
) {
  if (type == root2d_constraint_set::name) {
    return root2d_constraint_set::from_dict(std::move(skeleton), dico);
  } else if (type == fullbody_constraint_set::name) {
    return fullbody_constraint_set::from_dict(std::move(skeleton), dico);
  } else if (type == left_hand_constraint_set::name) {
    // 用 end_effector 的 from_dict 读取数据，但构造 left_hand 类型
    auto ee = end_effector_constraint_set::from_dict(skeleton, dico);
    return std::make_shared<left_hand_constraint_set>(
        skeleton, std::move(ee->frame_indices_), std::move(ee->global_joints_positions_),
        std::move(ee->global_joints_rots_), std::move(ee->smooth_root_2d_)
    );
  } else if (type == right_hand_constraint_set::name) {
    auto ee = end_effector_constraint_set::from_dict(skeleton, dico);
    return std::make_shared<right_hand_constraint_set>(
        skeleton, std::move(ee->frame_indices_), std::move(ee->global_joints_positions_),
        std::move(ee->global_joints_rots_), std::move(ee->smooth_root_2d_)
    );
  } else if (type == left_foot_constraint_set::name) {
    auto ee = end_effector_constraint_set::from_dict(skeleton, dico);
    return std::make_shared<left_foot_constraint_set>(
        skeleton, std::move(ee->frame_indices_), std::move(ee->global_joints_positions_),
        std::move(ee->global_joints_rots_), std::move(ee->smooth_root_2d_)
    );
  } else if (type == right_foot_constraint_set::name) {
    auto ee = end_effector_constraint_set::from_dict(skeleton, dico);
    return std::make_shared<right_foot_constraint_set>(
        skeleton, std::move(ee->frame_indices_), std::move(ee->global_joints_positions_),
        std::move(ee->global_joints_rots_), std::move(ee->smooth_root_2d_)
    );
  } else if (type == end_effector_constraint_set::name) {
    return end_effector_constraint_set::from_dict(std::move(skeleton), dico);
  }
  DOODLE_CHICK(false, "未知的约束类型: {}", type);
  return {};  // unreachable
}
}  // namespace detail

// ======================================================================
// Root2DConstraintSet
// ======================================================================

root2d_constraint_set::root2d_constraint_set(
    std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, MatrixX2fRow smooth_root_2d,
    MatrixX2fRow global_root_heading
)
    : skeleton_(std::move(skeleton)),
      frame_indices_(std::move(frame_indices)),
      smooth_root_2d_(std::move(smooth_root_2d)),
      global_root_heading_(std::move(global_root_heading)) {
  // smooth_root_2d 应为 [K, 2]
  DOODLE_CHICK(smooth_root_2d_.cols() == 2, "root2d: smooth_root_2d 列数应为 2，实际为 {}", smooth_root_2d_.cols());
  DOODLE_CHICK(
      frame_indices_.size() == smooth_root_2d_.rows(), "root2d: frame_indices 大小 {} 与 smooth_root_2d 行数 {} 不匹配",
      frame_indices_.size(), smooth_root_2d_.rows()
  );
  if (global_root_heading_.size() > 0) {
    DOODLE_CHICK(
        global_root_heading_.cols() == 2, "root2d: global_root_heading 列数应为 2，实际为 {}",
        global_root_heading_.cols()
    );
    DOODLE_CHICK(
        global_root_heading_.rows() == frame_indices_.size(),
        "root2d: global_root_heading 行数 {} 与 frame_indices 大小 {} 不匹配", global_root_heading_.rows(),
        frame_indices_.size()
    );
  }
}

void root2d_constraint_set::update_constraints(
    std::unordered_map<std::string, std::vector<MatrixXfRow>>& data_dict,
    std::unordered_map<std::string, std::vector<Eigen::VectorXi>>& index_dict
) const {
  data_dict["smooth_root_2d"].push_back(smooth_root_2d_);
  index_dict["smooth_root_2d"].push_back(frame_indices_);

  if (global_root_heading_.size() > 0) {
    data_dict["global_root_heading"].push_back(global_root_heading_);
    index_dict["global_root_heading"].push_back(frame_indices_);
  }
}

std::shared_ptr<constraint_set_base> root2d_constraint_set::crop_move(std::int64_t start, std::int64_t end) const {
  // 创建掩码：frame_indices 中在 [start, end) 范围内的帧
  const Eigen::Index K = frame_indices_.size();
  std::vector<Eigen::Index> mask_indices;
  mask_indices.reserve(static_cast<std::size_t>(K));
  for (Eigen::Index i = 0; i < K; ++i) {
    const auto idx = frame_indices_(i);
    if (idx >= start && idx < end) {
      mask_indices.push_back(i);
    }
  }

  const Eigen::Index new_K = static_cast<Eigen::Index>(mask_indices.size());
  Eigen::VectorXi new_frame_indices(new_K);
  MatrixXfRow new_smooth_root_2d(new_K, 2);
  MatrixXfRow new_global_root_heading;

  for (Eigen::Index i = 0; i < new_K; ++i) {
    const Eigen::Index src    = mask_indices[static_cast<std::size_t>(i)];
    new_frame_indices(i)      = frame_indices_(src) - static_cast<std::int64_t>(start);
    new_smooth_root_2d.row(i) = smooth_root_2d_.row(src);
  }

  if (global_root_heading_.size() > 0) {
    new_global_root_heading.resize(new_K, 2);
    for (Eigen::Index i = 0; i < new_K; ++i) {
      new_global_root_heading.row(i) = global_root_heading_.row(mask_indices[static_cast<std::size_t>(i)]);
    }
  }

  return std::make_shared<root2d_constraint_set>(
      skeleton_, new_frame_indices, new_smooth_root_2d, new_global_root_heading
  );
}

void root2d_constraint_set::to(const std::shared_ptr<skeleton_base>& skel) {
  if (skel) skeleton_ = skel;
}

void root2d_constraint_set::move(const Eigen::RowVector3f& offset) {
  smooth_root_2d_.col(0).array() += offset.x();
  smooth_root_2d_.col(1).array() += offset.z();
}

std::shared_ptr<root2d_constraint_set> root2d_constraint_set::from_dict(
    std::shared_ptr<skeleton_base> skeleton, const nlohmann::json& dico
) {
  Eigen::VectorXi frame_indices =
      json_to_eigen_matrix<std::int64_t, Eigen::Dynamic>(dico.at("frame_indices")).cast<int>();

  // smooth_root_2d:  为 [K, 3] (x, y, z)，但我们只取 (x, z) 作为约束, y 给 root_y_pos_ 使用
  MatrixX3fRow raw_smooth = json_to_eigen_matrix<float, Eigen::Dynamic, 3>(dico.at("smooth_root"));

  for (Eigen::Index i = 0; i < raw_smooth.rows(); ++i) {
    raw_smooth(i, 1) = raw_smooth(i, 2);  // 将 z 复制到第二列
  }
  raw_smooth.conservativeResize(raw_smooth.rows(), 2);  // 只保留前两列 (x, z)

  MatrixX2fRow global_root_heading;
  if (dico.contains("global_root_heading")) {
    global_root_heading = json_to_eigen_matrix<float, Eigen::Dynamic, 2>(dico.at("global_root_heading"));
  }

  return std::make_shared<root2d_constraint_set>(
      std::move(skeleton), std::move(frame_indices), std::move(raw_smooth), std::move(global_root_heading)
  );
}

// ======================================================================
// FullBodyConstraintSet
// ======================================================================

fullbody_constraint_set::fullbody_constraint_set(
    std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, MatrixXfRow global_joints_positions,
    MatrixXfRow global_joints_rots, MatrixX2fRow smooth_root_2d
)
    : skeleton_(std::move(skeleton)),
      frame_indices_(std::move(frame_indices)),
      global_joints_positions_(std::move(global_joints_positions)),
      global_joints_rots_(std::move(global_joints_rots)),
      smooth_root_2d_(std::move(smooth_root_2d)) {
  DOODLE_CHICK(skeleton_, "fullbody: skeleton 为空");
  const Eigen::Index K = frame_indices_.size();
  DOODLE_CHICK(K > 0, "fullbody: frame_indices 为空");

  const std::int64_t J = skeleton_->nbjoints_;
  DOODLE_CHICK(
      global_joints_positions_.rows() == K, "fullbody: global_joints_positions 行数 {} != K {}",
      global_joints_positions_.rows(), K
  );
  DOODLE_CHICK(
      global_joints_positions_.cols() == J * 3, "fullbody: global_joints_positions 列数 {} != J*3 {}",
      global_joints_positions_.cols(), J * 3
  );
  DOODLE_CHICK(
      global_joints_rots_.rows() == K, "fullbody: global_joints_rots 行数 {} != K {}", global_joints_rots_.rows(), K
  );
  DOODLE_CHICK(
      global_joints_rots_.cols() == J * 9, "fullbody: global_joints_rots 列数 {} != J*9 {}", global_joints_rots_.cols(),
      J * 9
  );

  // 如果没有提供 smooth_root_2d，从根关节位置提取
  if (smooth_root_2d_.size() == 0) {
    const Eigen::Index root_idx = skeleton_->root_idx_;
    smooth_root_2d_.resize(K, 2);
    for (Eigen::Index i = 0; i < K; ++i) {
      smooth_root_2d_(i, 0) = global_joints_positions_(i, root_idx * 3 + 0);
      smooth_root_2d_(i, 1) = global_joints_positions_(i, root_idx * 3 + 2);
    }
  } else {
    DOODLE_CHICK(smooth_root_2d_.cols() == 2, "fullbody: smooth_root_2d 列数应为 2，实际为 {}", smooth_root_2d_.cols());
    DOODLE_CHICK(smooth_root_2d_.rows() == K, "fullbody: smooth_root_2d 行数 {} != K {}", smooth_root_2d_.rows(), K);
  }

  // 计算根 Y 位置
  root_y_pos_.resize(K);
  const Eigen::Index root_idx = skeleton_->root_idx_;
  for (Eigen::Index i = 0; i < K; ++i) {
    root_y_pos_(i) = global_joints_positions_(i, root_idx * 3 + 1);
  }

  // 计算全局朝向
  global_root_heading_ = detail::compute_global_heading_from_positions(global_joints_positions_, *skeleton_);
}

void fullbody_constraint_set::update_constraints(
    std::unordered_map<std::string, std::vector<MatrixXfRow>>& data_dict,
    std::unordered_map<std::string, std::vector<Eigen::VectorXi>>& index_dict
) const {
  const std::int64_t J = skeleton_->nbjoints_;

  // 构建帧×关节对索引
  const Eigen::Index K = frame_indices_.size();
  Eigen::VectorXi indices_lst(K * J);
  for (Eigen::Index i = 0; i < K; ++i) {
    for (std::int64_t j = 0; j < J; ++j) {
      indices_lst(i * J + j) = frame_indices_(i);
    }
  }

  data_dict["global_joints_positions"].push_back(global_joints_positions_);
  index_dict["global_joints_positions"].push_back(indices_lst);

  data_dict["smooth_root_2d"].push_back(smooth_root_2d_);
  index_dict["smooth_root_2d"].push_back(frame_indices_);

  data_dict["root_y_pos"].push_back(root_y_pos_);
  index_dict["root_y_pos"].push_back(frame_indices_);

  data_dict["global_root_heading"].push_back(global_root_heading_);
  index_dict["global_root_heading"].push_back(frame_indices_);
}

std::shared_ptr<constraint_set_base> fullbody_constraint_set::crop_move(std::int64_t start, std::int64_t end) const {
  const Eigen::Index K = frame_indices_.size();
  std::vector<Eigen::Index> mask_indices;
  mask_indices.reserve(static_cast<std::size_t>(K));
  for (Eigen::Index i = 0; i < K; ++i) {
    const auto idx = frame_indices_(i);
    if (idx >= start && idx < end) {
      mask_indices.push_back(i);
    }
  }

  const Eigen::Index new_K = static_cast<Eigen::Index>(mask_indices.size());
  Eigen::VectorXi new_frame_indices(new_K);
  MatrixXfRow new_positions(new_K, global_joints_positions_.cols());
  MatrixXfRow new_rots(new_K, global_joints_rots_.cols());
  MatrixXfRow new_smooth(new_K, 2);

  for (Eigen::Index i = 0; i < new_K; ++i) {
    const Eigen::Index src = mask_indices[static_cast<std::size_t>(i)];
    new_frame_indices(i)   = frame_indices_(src) - static_cast<std::int64_t>(start);
    new_positions.row(i)   = global_joints_positions_.row(src);
    new_rots.row(i)        = global_joints_rots_.row(src);
    new_smooth.row(i)      = smooth_root_2d_.row(src);
  }

  return std::make_shared<fullbody_constraint_set>(skeleton_, new_frame_indices, new_positions, new_rots, new_smooth);
}

void fullbody_constraint_set::to(const std::shared_ptr<skeleton_base>& skel) {
  if (skel) skeleton_ = skel;
}

void fullbody_constraint_set::move(const Eigen::RowVector3f& offset) {
  const Eigen::Index J = skeleton_->nbjoints_;
  for (Eigen::Index j = 0; j < J; ++j) {
    global_joints_positions_.col(j * 3 + 0).array() += offset.x();
    global_joints_positions_.col(j * 3 + 1).array() += offset.y();
    global_joints_positions_.col(j * 3 + 2).array() += offset.z();
  }
  smooth_root_2d_.col(0).array() += offset.x();
  smooth_root_2d_.col(1).array() += offset.z();
}

std::shared_ptr<fullbody_constraint_set> fullbody_constraint_set::from_dict(
    std::shared_ptr<skeleton_base> skeleton, const nlohmann::json& dico
) {
  DOODLE_CHICK(skeleton, "fullbody::from_dict: skeleton 为空");

  Eigen::VectorXi frame_indices =
      json_to_eigen_matrix<std::int64_t, Eigen::Dynamic>(dico.at("frame_indices")).cast<int>();

  // 加载局部旋转（轴角）: JSON 可能是 [K, J, 3] 3D 或 [K*J, 3] 2D 格式
  // json_to_eigen_matrix 统一 flatten 为 [K*J, 3]
  MatrixXfRow local_rot_aa      = json_to_eigen_matrix<float, Eigen::Dynamic, 3>(dico.at("local_joints_rot"));
  // [K*J, 3] → axis_angle → [K*J, 9]
  MatrixXfRow local_rot_flat    = axis_angle_to_matrix(local_rot_aa);

  // reshape [K*J, 9] → [K, J*9] 用于 FK
  const std::int64_t J          = skeleton->nbjoints_;
  const Eigen::Index total_rows = local_rot_flat.rows();
  const Eigen::Index K          = total_rows / J;
  DOODLE_CHICK(total_rows == K * J, "fullbody: local_joints_rot 行数 {} 不是 J={} 的整数倍", total_rows, J);

  MatrixXfRow local_rot_mats(K, J * 9);
  for (Eigen::Index i = 0; i < K; ++i) {
    for (std::int64_t j = 0; j < J; ++j) {
      local_rot_mats.block(i, j * 9, 1, 9) = local_rot_flat.row(i * J + j);
    }
  }

  MatrixXfRow root_positions = json_to_eigen_matrix<float, Eigen::Dynamic, 3>(dico.at("root_positions"));
  DOODLE_CHICK(
      root_positions.rows() == K, "fullbody: root_positions 行数 {} 与帧数 K={} 不匹配", root_positions.rows(), K
  );

  // FK: 得到全局旋转和关节位置
  auto fk_result = skeleton->fk(local_rot_mats, root_positions);

  MatrixX2fRow smooth_root_2d;
  if (dico.contains("smooth_root")) {
    smooth_root_2d = json_to_eigen_matrix<float, Eigen::Dynamic, 3>(dico.at("smooth_root"));
    for (Eigen::Index i = 0; i < smooth_root_2d.rows(); ++i)
      smooth_root_2d(i, 1) = smooth_root_2d(i, 2);  // 将 z 复制到第二列

    smooth_root_2d.conservativeResize(smooth_root_2d.rows(), 2);  // 只保留前两列 (x, z)
  }

  return std::make_shared<fullbody_constraint_set>(
      std::move(skeleton), std::move(frame_indices), std::move(fk_result.posed_joints),
      std::move(fk_result.global_rot_mats), std::move(smooth_root_2d)
  );
}

// ======================================================================
// EndEffectorConstraintSet
// ======================================================================

end_effector_constraint_set::end_effector_constraint_set(
    std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, MatrixXfRow global_joints_positions,
    MatrixXfRow global_joints_rots, MatrixX2fRow smooth_root_2d, std::vector<std::string> joint_names
)
    : skeleton_(std::move(skeleton)),
      frame_indices_(std::move(frame_indices)),
      joint_names_(std::move(joint_names)),
      global_joints_positions_(std::move(global_joints_positions)),
      global_joints_rots_(std::move(global_joints_rots)),
      smooth_root_2d_(std::move(smooth_root_2d)) {
  DOODLE_CHICK(skeleton_, "end_effector: skeleton 为空");
  DOODLE_CHICK(!joint_names_.empty(), "end_effector: joint_names 为空");

  const Eigen::Index K = frame_indices_.size();
  DOODLE_CHICK(K > 0, "end_effector: frame_indices 为空");

  // 通过 skeleton 展开关节名称（获取 pos 和 rot 对应的关节名称链）
  // 注意：C++ skeleton 当前没有 expand_joint_names，这里简化：
  //   使用 joint_names_ 直接查找 bone_index
  pos_indices_.resize(static_cast<Eigen::Index>(joint_names_.size()));
  rot_indices_.resize(static_cast<Eigen::Index>(joint_names_.size()));
  for (std::size_t i = 0; i < joint_names_.size(); ++i) {
    const auto it = skeleton_->bone_index_.find(joint_names_[i]);
    DOODLE_CHICK(it != skeleton_->bone_index_.end(), "end_effector: 关节名 '{}' 未在 skeleton 中找到", joint_names_[i]);
    pos_indices_(static_cast<Eigen::Index>(i)) = it->second;
    rot_indices_(static_cast<Eigen::Index>(i)) = it->second;
  }

  // 验证位置/旋转矩阵列数
  const std::int64_t n_pos = pos_indices_.size();
  const std::int64_t n_rot = rot_indices_.size();
  DOODLE_CHICK(
      global_joints_positions_.cols() == n_pos * 3, "end_effector: positions 列数 {} != n_pos*3 {}",
      global_joints_positions_.cols(), n_pos * 3
  );
  DOODLE_CHICK(
      global_joints_rots_.cols() == n_rot * 9, "end_effector: rotations 列数 {} != n_rot*9 {}",
      global_joints_rots_.cols(), n_rot * 9
  );

  // 如果没有提供 smooth_root_2d，从根位置提取
  const Eigen::Index root_idx = skeleton_->root_idx_;
  if (smooth_root_2d_.size() == 0) {
    smooth_root_2d_.resize(K, 2);
    for (Eigen::Index i = 0; i < K; ++i) {
      smooth_root_2d_(i, 0) = global_joints_positions_(i, root_idx * 3 + 0);
      smooth_root_2d_(i, 1) = global_joints_positions_(i, root_idx * 3 + 2);
    }
  }

  // 根 Y 位置
  root_y_pos_.resize(K);
  for (Eigen::Index i = 0; i < K; ++i) {
    root_y_pos_(i) = global_joints_positions_(i, root_idx * 3 + 1);
  }

  // 计算全局朝向
  global_root_heading_ = detail::compute_global_heading_from_positions(global_joints_positions_, *skeleton_);
}

void end_effector_constraint_set::update_constraints(
    std::unordered_map<std::string, std::vector<MatrixXfRow>>& data_dict,
    std::unordered_map<std::string, std::vector<Eigen::VectorXi>>& index_dict
) const {
  const Eigen::Index K     = frame_indices_.size();
  const std::int64_t n_pos = pos_indices_.size();
  const std::int64_t n_rot = rot_indices_.size();

  // 位置索引对
  Eigen::VectorXi pos_indices_real(K * n_pos);
  for (Eigen::Index i = 0; i < K; ++i) {
    for (std::int64_t j = 0; j < n_pos; ++j) {
      pos_indices_real(i * static_cast<Eigen::Index>(n_pos) + j) = frame_indices_(i);
    }
  }
  data_dict["global_joints_positions"].push_back(global_joints_positions_);
  index_dict["global_joints_positions"].push_back(pos_indices_real);

  // 旋转索引对
  Eigen::VectorXi rot_indices_real(K * n_rot);
  for (Eigen::Index i = 0; i < K; ++i) {
    for (std::int64_t j = 0; j < n_rot; ++j) {
      rot_indices_real(i * static_cast<Eigen::Index>(n_rot) + j) = frame_indices_(i);
    }
  }
  data_dict["global_joints_rots"].push_back(global_joints_rots_);
  index_dict["global_joints_rots"].push_back(rot_indices_real);

  data_dict["smooth_root_2d"].push_back(smooth_root_2d_);
  index_dict["smooth_root_2d"].push_back(frame_indices_);

  data_dict["root_y_pos"].push_back(root_y_pos_);
  index_dict["root_y_pos"].push_back(frame_indices_);

  data_dict["global_root_heading"].push_back(global_root_heading_);
  index_dict["global_root_heading"].push_back(frame_indices_);
}

std::shared_ptr<constraint_set_base> end_effector_constraint_set::crop_move(
    std::int64_t start, std::int64_t end
) const {
  const Eigen::Index K = frame_indices_.size();
  std::vector<Eigen::Index> mask_indices;
  mask_indices.reserve(static_cast<std::size_t>(K));
  for (Eigen::Index i = 0; i < K; ++i) {
    const auto idx = frame_indices_(i);
    if (idx >= start && idx < end) {
      mask_indices.push_back(i);
    }
  }

  const Eigen::Index new_K = static_cast<Eigen::Index>(mask_indices.size());
  Eigen::VectorXi new_frame_indices(new_K);
  MatrixXfRow new_positions(new_K, global_joints_positions_.cols());
  MatrixXfRow new_rots(new_K, global_joints_rots_.cols());
  MatrixXfRow new_smooth(new_K, 2);

  for (Eigen::Index i = 0; i < new_K; ++i) {
    const Eigen::Index src = mask_indices[static_cast<std::size_t>(i)];
    new_frame_indices(i)   = frame_indices_(src) - static_cast<std::int64_t>(start);
    new_positions.row(i)   = global_joints_positions_.row(src);
    new_rots.row(i)        = global_joints_rots_.row(src);
    new_smooth.row(i)      = smooth_root_2d_.row(src);
  }

  return std::make_shared<end_effector_constraint_set>(
      skeleton_, new_frame_indices, new_positions, new_rots, new_smooth, joint_names_
  );
}

void end_effector_constraint_set::to(const std::shared_ptr<skeleton_base>& skel) {
  if (skel) skeleton_ = skel;
}

void end_effector_constraint_set::move(const Eigen::RowVector3f& offset) {
  const Eigen::Index n_pos = pos_indices_.size();
  for (Eigen::Index j = 0; j < n_pos; ++j) {
    global_joints_positions_.col(j * 3 + 0).array() += offset.x();
    global_joints_positions_.col(j * 3 + 1).array() += offset.y();
    global_joints_positions_.col(j * 3 + 2).array() += offset.z();
  }
  smooth_root_2d_.col(0).array() += offset.x();
  smooth_root_2d_.col(1).array() += offset.z();
}

std::shared_ptr<end_effector_constraint_set> end_effector_constraint_set::from_dict(
    std::shared_ptr<skeleton_base> skeleton, const nlohmann::json& dico
) {
  DOODLE_CHICK(skeleton, "end_effector::from_dict: skeleton 为空");

  Eigen::VectorXi frame_indices =
      json_to_eigen_matrix<std::int64_t, Eigen::Dynamic>(dico.at("frame_indices")).cast<int>();

  // 加载局部旋转（轴角）: JSON 可能是 [K, J, 3] 3D 或 [K*J, 3] 2D 格式
  MatrixXfRow local_rot_aa      = json_to_eigen_matrix<float, Eigen::Dynamic, 3>(dico.at("local_joints_rot"));
  // [K*J, 3] → axis_angle → [K*J, 9]
  MatrixXfRow local_rot_flat    = axis_angle_to_matrix(local_rot_aa);

  // reshape [K*J, 9] → [K, J*9] 用于 FK
  const std::int64_t J          = skeleton->nbjoints_;
  const Eigen::Index total_rows = local_rot_flat.rows();
  const Eigen::Index K          = total_rows / J;
  DOODLE_CHICK(total_rows == K * J, "end_effector: local_joints_rot 行数 {} 不是 J={} 的整数倍", total_rows, J);

  MatrixXfRow local_rot_mats(K, J * 9);
  for (Eigen::Index i = 0; i < K; ++i) {
    for (std::int64_t j = 0; j < J; ++j) {
      local_rot_mats.block(i, j * 9, 1, 9) = local_rot_flat.row(i * J + j);
    }
  }

  MatrixXfRow root_positions = json_to_eigen_matrix<float, Eigen::Dynamic, 3>(dico.at("root_positions"));
  DOODLE_CHICK(
      root_positions.rows() == K, "end_effector: root_positions 行数 {} 与帧数 K={} 不匹配", root_positions.rows(), K
  );

  // FK
  auto fk_result = skeleton->fk(local_rot_mats, root_positions);

  MatrixXfRow smooth_root_2d;
  if (dico.contains("smooth_root")) {
    smooth_root_2d = json_to_eigen_matrix<float, Eigen::Dynamic, 3>(dico.at("smooth_root"));
    for (Eigen::Index i = 0; i < smooth_root_2d.rows(); ++i)
      smooth_root_2d(i, 1) = smooth_root_2d(i, 2);                // 将 z 复制到第二列
    smooth_root_2d.conservativeResize(smooth_root_2d.rows(), 2);  // 只保留前两列 (x, z)
  }

  std::vector<std::string> joint_names;
  if (dico.contains("joint_names")) {
    joint_names = dico.at("joint_names").get<std::vector<std::string>>();
  }

  return std::make_shared<end_effector_constraint_set>(
      std::move(skeleton), std::move(frame_indices), std::move(fk_result.posed_joints),
      std::move(fk_result.global_rot_mats), std::move(smooth_root_2d), std::move(joint_names)
  );
}

// ======================================================================
// 辅助：约束类型名称获取
// ======================================================================

std::string get_constraint_type_name(const constraint_set_var& constraint) {
  return std::visit([](const auto& c) -> std::string { return c.name; }, constraint);
}

// ======================================================================
// 加载约束列表
// ======================================================================

std::vector<constraint_set_ptr> load_constraints_lst(const FSys::path& path, std::shared_ptr<skeleton_base> skeleton) {
  DOODLE_CHICK(FSys::exists(path), "约束 JSON 文件不存在: {}", path.string());

  SPDLOG_INFO("Loading constraints from {}", path.string());
  auto json_data = nlohmann::json::parse(FSys::ifstream{path});
  return load_constraints_lst_from_json(json_data, std::move(skeleton));
}

std::vector<constraint_set_ptr> load_constraints_lst_from_json(
    const nlohmann::json& json_data, std::shared_ptr<skeleton_base> skeleton
) {
  DOODLE_CHICK(json_data.is_array(), "约束 JSON 数据应为数组");

  std::vector<constraint_set_ptr> constraints;
  constraints.reserve(json_data.size());

  for (const auto& el : json_data) {
    const std::string type = el.at("type").get<std::string>();
    auto c                 = detail::make_constraint_from_type(type, skeleton, el);
    constraints.push_back(std::move(c));
  }

  SPDLOG_INFO("Loaded {} constraints from JSON", constraints.size());
  return constraints;
}

}  // namespace doodle::ai
