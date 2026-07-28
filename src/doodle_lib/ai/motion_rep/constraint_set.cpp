//
// Created by TD on 25-7-28.
//
#include "constraint_set.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/ai/motion_rep/feature_utils.h>
#include <doodle_lib/ai/motion_rep/geometry.h>

#include <fstream>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace doodle::ai {

// ======================================================================
// 辅助函数：计算全局朝向 (cos, sin) 用于约束初始化
// ======================================================================
namespace detail {
Eigen::MatrixXf compute_global_heading_from_positions(
    const Eigen::MatrixXf& global_joints_positions, const skeleton_base& skeleton
) {
  const Eigen::Index K = global_joints_positions.rows();
  const Eigen::MatrixXf heading_angle =
      compute_heading_angle(global_joints_positions, skeleton, 1, K);
  // heading_angle: [1, K]
  Eigen::MatrixXf heading(K, 2);
  for (Eigen::Index i = 0; i < K; ++i) {
    heading(i, 0) = std::cos(heading_angle(0, i));
    heading(i, 1) = std::sin(heading_angle(0, i));
  }
  return heading;
}

// 根据 JSON 中 type 字段构造正确的约束变体
constraint_set_var make_constraint_from_type(
    const std::string& type, std::shared_ptr<skeleton_base> skeleton, const nlohmann::json& dico
) {
  if (type == root2d_constraint_set::name) {
    return root2d_constraint_set::from_dict(std::move(skeleton), dico);
  } else if (type == fullbody_constraint_set::name) {
    return fullbody_constraint_set::from_dict(std::move(skeleton), dico);
  } else if (type == left_hand_constraint_set::name) {
    // 用 end_effector 的 from_dict 读取数据，但构造 left_hand 类型
    auto ee = end_effector_constraint_set::from_dict(skeleton, dico);
    return left_hand_constraint_set{
        skeleton, std::move(ee.frame_indices_), std::move(ee.global_joints_positions_),
        std::move(ee.global_joints_rots_), std::move(ee.smooth_root_2d_)
    };
  } else if (type == right_hand_constraint_set::name) {
    auto ee = end_effector_constraint_set::from_dict(skeleton, dico);
    return right_hand_constraint_set{
        skeleton, std::move(ee.frame_indices_), std::move(ee.global_joints_positions_),
        std::move(ee.global_joints_rots_), std::move(ee.smooth_root_2d_)
    };
  } else if (type == left_foot_constraint_set::name) {
    auto ee = end_effector_constraint_set::from_dict(skeleton, dico);
    return left_foot_constraint_set{
        skeleton, std::move(ee.frame_indices_), std::move(ee.global_joints_positions_),
        std::move(ee.global_joints_rots_), std::move(ee.smooth_root_2d_)
    };
  } else if (type == right_foot_constraint_set::name) {
    auto ee = end_effector_constraint_set::from_dict(skeleton, dico);
    return right_foot_constraint_set{
        skeleton, std::move(ee.frame_indices_), std::move(ee.global_joints_positions_),
        std::move(ee.global_joints_rots_), std::move(ee.smooth_root_2d_)
    };
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
    std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices, Eigen::MatrixXf smooth_root_2d,
    Eigen::MatrixXf global_root_heading
)
    : skeleton_(std::move(skeleton)),
      frame_indices_(std::move(frame_indices)),
      smooth_root_2d_(std::move(smooth_root_2d)),
      global_root_heading_(std::move(global_root_heading)) {
  // smooth_root_2d 应为 [K, 2]
  DOODLE_CHICK(
      smooth_root_2d_.cols() == 2, "root2d: smooth_root_2d 列数应为 2，实际为 {}", smooth_root_2d_.cols()
  );
  DOODLE_CHICK(
      frame_indices_.size() == smooth_root_2d_.rows(),
      "root2d: frame_indices 大小 {} 与 smooth_root_2d 行数 {} 不匹配", frame_indices_.size(), smooth_root_2d_.rows()
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
    std::unordered_map<std::string, std::vector<Eigen::MatrixXf>>& data_dict,
    std::unordered_map<std::string, std::vector<Eigen::VectorXi>>& index_dict
) const {
  data_dict["smooth_root_2d"].push_back(smooth_root_2d_);
  index_dict["smooth_root_2d"].push_back(frame_indices_);

  if (global_root_heading_.size() > 0) {
    data_dict["global_root_heading"].push_back(global_root_heading_);
    index_dict["global_root_heading"].push_back(frame_indices_);
  }
}

root2d_constraint_set root2d_constraint_set::crop_move(std::int64_t start, std::int64_t end) const {
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
  Eigen::MatrixXf new_smooth_root_2d(new_K, 2);
  Eigen::MatrixXf new_global_root_heading;

  for (Eigen::Index i = 0; i < new_K; ++i) {
    const Eigen::Index src = mask_indices[static_cast<std::size_t>(i)];
    new_frame_indices(i)   = frame_indices_(src) - static_cast<std::int64_t>(start);
    new_smooth_root_2d.row(i) = smooth_root_2d_.row(src);
  }

  if (global_root_heading_.size() > 0) {
    new_global_root_heading.resize(new_K, 2);
    for (Eigen::Index i = 0; i < new_K; ++i) {
      new_global_root_heading.row(i) = global_root_heading_.row(mask_indices[static_cast<std::size_t>(i)]);
    }
  }

  return root2d_constraint_set(skeleton_, new_frame_indices, new_smooth_root_2d, new_global_root_heading);
}

nlohmann::json root2d_constraint_set::get_save_info() const {
  nlohmann::json out;
  out["type"]             = name;
  out["frame_indices"]    = eigen_matrix_to_json(frame_indices_);
  out["smooth_root_2d"]   = eigen_matrix_to_json(smooth_root_2d_);
  if (global_root_heading_.size() > 0) {
    out["global_root_heading"] = eigen_matrix_to_json(global_root_heading_);
  }
  return out;
}

void root2d_constraint_set::to(const std::shared_ptr<skeleton_base>& skel) {
  if (skel) skeleton_ = skel;
}

root2d_constraint_set root2d_constraint_set::from_dict(
    std::shared_ptr<skeleton_base> skeleton, const nlohmann::json& dico
) {
  Eigen::VectorXi frame_indices =
      json_to_eigen_matrix<std::int64_t>(dico.at("frame_indices")).cast<int>();

  // smooth_root_2d: 可能为 [K, 2] 或 [K, 3]（3D 时取前两列）
  Eigen::MatrixXf raw_smooth = json_to_eigen_matrix<float>(dico.at("smooth_root_2d"));
  Eigen::MatrixXf smooth_root_2d;
  if (raw_smooth.cols() == 3) {
    smooth_root_2d.resize(raw_smooth.rows(), 2);
    smooth_root_2d.col(0) = raw_smooth.col(0);
    smooth_root_2d.col(1) = raw_smooth.col(1);
  } else {
    smooth_root_2d = std::move(raw_smooth);
  }

  Eigen::MatrixXf global_root_heading;
  if (dico.contains("global_root_heading")) {
    global_root_heading = json_to_eigen_matrix<float>(dico.at("global_root_heading"));
  }

  return root2d_constraint_set(
      std::move(skeleton), std::move(frame_indices), std::move(smooth_root_2d), std::move(global_root_heading)
  );
}

// ======================================================================
// FullBodyConstraintSet
// ======================================================================

fullbody_constraint_set::fullbody_constraint_set(
    std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices,
    Eigen::MatrixXf global_joints_positions, Eigen::MatrixXf global_joints_rots, Eigen::MatrixXf smooth_root_2d
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
      global_joints_rots_.rows() == K, "fullbody: global_joints_rots 行数 {} != K {}",
      global_joints_rots_.rows(), K
  );
  DOODLE_CHICK(
      global_joints_rots_.cols() == J * 9, "fullbody: global_joints_rots 列数 {} != J*9 {}",
      global_joints_rots_.cols(), J * 9
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
    DOODLE_CHICK(
        smooth_root_2d_.cols() == 2, "fullbody: smooth_root_2d 列数应为 2，实际为 {}", smooth_root_2d_.cols()
    );
    DOODLE_CHICK(
        smooth_root_2d_.rows() == K, "fullbody: smooth_root_2d 行数 {} != K {}", smooth_root_2d_.rows(), K
    );
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
    std::unordered_map<std::string, std::vector<Eigen::MatrixXf>>& data_dict,
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

fullbody_constraint_set fullbody_constraint_set::crop_move(std::int64_t start, std::int64_t end) const {
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
  Eigen::MatrixXf new_positions(new_K, global_joints_positions_.cols());
  Eigen::MatrixXf new_rots(new_K, global_joints_rots_.cols());
  Eigen::MatrixXf new_smooth(new_K, 2);

  for (Eigen::Index i = 0; i < new_K; ++i) {
    const Eigen::Index src = mask_indices[static_cast<std::size_t>(i)];
    new_frame_indices(i)   = frame_indices_(src) - static_cast<std::int64_t>(start);
    new_positions.row(i)   = global_joints_positions_.row(src);
    new_rots.row(i)        = global_joints_rots_.row(src);
    new_smooth.row(i)      = smooth_root_2d_.row(src);
  }

  return fullbody_constraint_set(skeleton_, new_frame_indices, new_positions, new_rots, new_smooth);
}

nlohmann::json fullbody_constraint_set::get_save_info() const {
  // 将全局旋转转换为局部旋转: [K, J*9] → [K, J*9]
  Eigen::MatrixXf local_joints_rot = skeleton_->global_rots_to_local_rots(global_joints_rots_);

  // 转换为轴角格式: 先将 [K, J*9] reshape 为 [K*J, 9]
  const Eigen::Index K        = frame_indices_.size();
  const std::int64_t J        = skeleton_->nbjoints_;
  Eigen::MatrixXf local_rot_flat(K * J, 9);
  for (Eigen::Index i = 0; i < K; ++i) {
    for (std::int64_t j = 0; j < J; ++j) {
      local_rot_flat.row(i * J + j) = local_joints_rot.row(i).segment(j * 9, 9);
    }
  }
  // [K*J, 9] → axis_angle → [K*J, 3]
  Eigen::MatrixXf local_aa_flat = matrix_to_axis_angle(local_rot_flat);

  const Eigen::Index root_idx = skeleton_->root_idx_;
  Eigen::MatrixXf root_positions(K, 3);
  for (Eigen::Index i = 0; i < K; ++i) {
    root_positions.row(i) = global_joints_positions_.row(i).segment(root_idx * 3, 3);
  }

  nlohmann::json out;
  out["type"]             = name;
  out["frame_indices"]    = eigen_matrix_to_json(frame_indices_);
  out["local_joints_rot"] = eigen_matrix_to_json(local_aa_flat);
  out["root_positions"]   = eigen_matrix_to_json(root_positions);
  out["smooth_root_2d"]   = eigen_matrix_to_json(smooth_root_2d_);
  return out;
}

void fullbody_constraint_set::to(const std::shared_ptr<skeleton_base>& skel) {
  if (skel) skeleton_ = skel;
}

fullbody_constraint_set fullbody_constraint_set::from_dict(
    std::shared_ptr<skeleton_base> skeleton, const nlohmann::json& dico
) {
  DOODLE_CHICK(skeleton, "fullbody::from_dict: skeleton 为空");

  Eigen::VectorXi frame_indices =
      json_to_eigen_matrix<std::int64_t>(dico.at("frame_indices")).cast<int>();

  // 加载局部旋转（轴角）: JSON 可能是 [K, J, 3] 3D 或 [K*J, 3] 2D 格式
  // json_to_eigen_matrix 统一 flatten 为 [K*J, 3]
  Eigen::MatrixXf local_rot_aa = json_to_eigen_matrix<float>(dico.at("local_joints_rot"));
  // [K*J, 3] → axis_angle → [K*J, 9]
  Eigen::MatrixXf local_rot_flat = axis_angle_to_matrix(local_rot_aa);

  // reshape [K*J, 9] → [K, J*9] 用于 FK
  const std::int64_t J = skeleton->nbjoints_;
  const Eigen::Index total_rows = local_rot_flat.rows();
  const Eigen::Index K = total_rows / J;
  DOODLE_CHICK(total_rows == K * J, "fullbody: local_joints_rot 行数 {} 不是 J={} 的整数倍", total_rows, J);

  Eigen::MatrixXf local_rot_mats(K, J * 9);
  for (Eigen::Index i = 0; i < K; ++i) {
    for (std::int64_t j = 0; j < J; ++j) {
      local_rot_mats.block(i, j * 9, 1, 9) = local_rot_flat.row(i * J + j);
    }
  }

  // TODO: 30↔77 关节转换 — 当 skeleton 为 SOMASkeleton30 且 local_rot 为 77 关节时需要转换
  // 当前 C++ 版本暂未实现 SOMA 30↔77 转换，需在 skeleton 中增加对应方法后启用
  // if (local_rot_mats.rows() / 3 == 77 && skeleton->nbjoints_ == 30) {
  //   local_rot_mats = skeleton->from_SOMASkeleton77(local_rot_mats);
  // } else if (local_rot_mats.rows() / 3 == 30 && skeleton->nbjoints_ == 77) {
  //   local_rot_mats = skeleton->to_SOMASkeleton77(local_rot_mats);
  // }

  Eigen::MatrixXf root_positions = json_to_eigen_matrix<float>(dico.at("root_positions"));
  DOODLE_CHICK(root_positions.rows() == K, "fullbody: root_positions 行数 {} 与帧数 K={} 不匹配", root_positions.rows(), K);

  // FK: 得到全局旋转和关节位置
  auto fk_result = skeleton->fk(local_rot_mats, root_positions);

  Eigen::MatrixXf smooth_root_2d;
  if (dico.contains("smooth_root_2d")) {
    smooth_root_2d = json_to_eigen_matrix<float>(dico.at("smooth_root_2d"));
  }

  return fullbody_constraint_set(
      std::move(skeleton), std::move(frame_indices), std::move(fk_result.posed_joints),
      std::move(fk_result.global_rot_mats), std::move(smooth_root_2d)
  );
}

// ======================================================================
// EndEffectorConstraintSet
// ======================================================================

end_effector_constraint_set::end_effector_constraint_set(
    std::shared_ptr<skeleton_base> skeleton, Eigen::VectorXi frame_indices,
    Eigen::MatrixXf global_joints_positions, Eigen::MatrixXf global_joints_rots,
    Eigen::MatrixXf smooth_root_2d, std::vector<std::string> joint_names
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
    DOODLE_CHICK(
        it != skeleton_->bone_index_.end(), "end_effector: 关节名 '{}' 未在 skeleton 中找到",
        joint_names_[i]
    );
    pos_indices_(static_cast<Eigen::Index>(i)) = it->second;
    rot_indices_(static_cast<Eigen::Index>(i)) = it->second;
  }

  // 验证位置/旋转矩阵列数
  const std::int64_t n_pos = pos_indices_.size();
  const std::int64_t n_rot = rot_indices_.size();
  DOODLE_CHICK(
      global_joints_positions_.cols() == n_pos * 3,
      "end_effector: positions 列数 {} != n_pos*3 {}", global_joints_positions_.cols(), n_pos * 3
  );
  DOODLE_CHICK(
      global_joints_rots_.cols() == n_rot * 9,
      "end_effector: rotations 列数 {} != n_rot*9 {}", global_joints_rots_.cols(), n_rot * 9
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
    std::unordered_map<std::string, std::vector<Eigen::MatrixXf>>& data_dict,
    std::unordered_map<std::string, std::vector<Eigen::VectorXi>>& index_dict
) const {
  const Eigen::Index K = frame_indices_.size();
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

end_effector_constraint_set end_effector_constraint_set::crop_move(std::int64_t start, std::int64_t end) const {
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
  Eigen::MatrixXf new_positions(new_K, global_joints_positions_.cols());
  Eigen::MatrixXf new_rots(new_K, global_joints_rots_.cols());
  Eigen::MatrixXf new_smooth(new_K, 2);

  for (Eigen::Index i = 0; i < new_K; ++i) {
    const Eigen::Index src = mask_indices[static_cast<std::size_t>(i)];
    new_frame_indices(i)   = frame_indices_(src) - static_cast<std::int64_t>(start);
    new_positions.row(i)   = global_joints_positions_.row(src);
    new_rots.row(i)        = global_joints_rots_.row(src);
    new_smooth.row(i)      = smooth_root_2d_.row(src);
  }

  return end_effector_constraint_set(
      skeleton_, new_frame_indices, new_positions, new_rots, new_smooth, joint_names_
  );
}

nlohmann::json end_effector_constraint_set::get_save_info() const {
  // 将全局旋转转换为局部旋转: [K, J*9] → [K, J*9] (所有关节)
  Eigen::MatrixXf local_joints_rot = skeleton_->global_rots_to_local_rots(global_joints_rots_);

  // 转换为轴角格式: reshape [K, J*9] → [K*J, 9]
  const Eigen::Index K        = frame_indices_.size();
  const std::int64_t J        = skeleton_->nbjoints_;
  Eigen::MatrixXf local_rot_flat(K * J, 9);
  for (Eigen::Index i = 0; i < K; ++i) {
    for (std::int64_t j = 0; j < J; ++j) {
      local_rot_flat.row(i * J + j) = local_joints_rot.row(i).segment(j * 9, 9);
    }
  }
  // [K*J, 9] → axis_angle → [K*J, 3]
  Eigen::MatrixXf local_aa_flat = matrix_to_axis_angle(local_rot_flat);

  // 仅保存根位置 — 但 end-effector 中不一定有根关节的全量位置
  // 使用零填充
  Eigen::MatrixXf root_positions(K, 3);
  root_positions.setZero();

  nlohmann::json out;
  out["type"]             = name;
  out["frame_indices"]    = eigen_matrix_to_json(frame_indices_);
  out["local_joints_rot"] = eigen_matrix_to_json(local_aa_flat);
  out["root_positions"]   = eigen_matrix_to_json(root_positions);
  out["smooth_root_2d"]   = eigen_matrix_to_json(smooth_root_2d_);

  // 如果是基础 end_effector 类（不是左/右手/脚子类），保存 joint_names
  // 注意：在 C++ 中无法像 Python 那样用 hasattr 区分，所以统一保存
  out["joint_names"] = joint_names_;

  return out;
}

void end_effector_constraint_set::to(const std::shared_ptr<skeleton_base>& skel) {
  if (skel) skeleton_ = skel;
}

end_effector_constraint_set end_effector_constraint_set::from_dict(
    std::shared_ptr<skeleton_base> skeleton, const nlohmann::json& dico
) {
  DOODLE_CHICK(skeleton, "end_effector::from_dict: skeleton 为空");

  Eigen::VectorXi frame_indices =
      json_to_eigen_matrix<std::int64_t>(dico.at("frame_indices")).cast<int>();

  // 加载局部旋转（轴角）: JSON 可能是 [K, J, 3] 3D 或 [K*J, 3] 2D 格式
  Eigen::MatrixXf local_rot_aa    = json_to_eigen_matrix<float>(dico.at("local_joints_rot"));
  // [K*J, 3] → axis_angle → [K*J, 9]
  Eigen::MatrixXf local_rot_flat  = axis_angle_to_matrix(local_rot_aa);

  // reshape [K*J, 9] → [K, J*9] 用于 FK
  const std::int64_t J = skeleton->nbjoints_;
  const Eigen::Index total_rows = local_rot_flat.rows();
  const Eigen::Index K = total_rows / J;
  DOODLE_CHICK(total_rows == K * J, "end_effector: local_joints_rot 行数 {} 不是 J={} 的整数倍", total_rows, J);

  Eigen::MatrixXf local_rot_mats(K, J * 9);
  for (Eigen::Index i = 0; i < K; ++i) {
    for (std::int64_t j = 0; j < J; ++j) {
      local_rot_mats.block(i, j * 9, 1, 9) = local_rot_flat.row(i * J + j);
    }
  }

  // TODO: 30↔77 关节转换（同 fullbody）

  Eigen::MatrixXf root_positions = json_to_eigen_matrix<float>(dico.at("root_positions"));
  DOODLE_CHICK(root_positions.rows() == K, "end_effector: root_positions 行数 {} 与帧数 K={} 不匹配", root_positions.rows(), K);

  // FK
  auto fk_result = skeleton->fk(local_rot_mats, root_positions);

  Eigen::MatrixXf smooth_root_2d;
  if (dico.contains("smooth_root_2d")) {
    smooth_root_2d = json_to_eigen_matrix<float>(dico.at("smooth_root_2d"));
  }

  std::vector<std::string> joint_names;
  if (dico.contains("joint_names")) {
    joint_names = dico.at("joint_names").get<std::vector<std::string>>();
  }

  return end_effector_constraint_set(
      std::move(skeleton), std::move(frame_indices), std::move(fk_result.posed_joints),
      std::move(fk_result.global_rot_mats), std::move(smooth_root_2d), std::move(joint_names)
  );
}

// ======================================================================
// 辅助：约束类型名称获取
// ======================================================================

std::string get_constraint_type_name(const constraint_set_var& constraint) {
  return std::visit(
      [](const auto& c) -> std::string { return c.name; }, constraint
  );
}

// ======================================================================
// 加载约束列表
// ======================================================================

std::vector<constraint_set_var> load_constraints_lst(
    const FSys::path& path, std::shared_ptr<skeleton_base> skeleton
) {
  DOODLE_CHICK(FSys::exists(path), "约束 JSON 文件不存在: {}", path.string());

  SPDLOG_INFO("Loading constraints from {}", path.string());
  auto json_data = nlohmann::json::parse(FSys::ifstream{path});
  return load_constraints_lst_from_json(json_data, std::move(skeleton));
}

std::vector<constraint_set_var> load_constraints_lst_from_json(
    const nlohmann::json& json_data, std::shared_ptr<skeleton_base> skeleton
) {
  DOODLE_CHICK(json_data.is_array(), "约束 JSON 数据应为数组");

  std::vector<constraint_set_var> constraints;
  constraints.reserve(json_data.size());

  for (const auto& el : json_data) {
    const std::string type = el.at("type").get<std::string>();
    auto c = detail::make_constraint_from_type(type, skeleton, el);
    constraints.push_back(std::move(c));
  }

  SPDLOG_INFO("Loaded {} constraints from JSON", constraints.size());
  return constraints;
}

// ======================================================================
// 保存约束列表
// ======================================================================

nlohmann::json save_constraints_lst_to_json(const std::vector<constraint_set_var>& constraints_lst) {
  if (constraints_lst.empty()) {
    SPDLOG_INFO("The constraints list is empty. Skip saving");
    return nullptr;
  }

  nlohmann::json to_save = nlohmann::json::array();

  for (const auto& constraint : constraints_lst) {
    nlohmann::json info = std::visit(
        [](const auto& c) -> nlohmann::json { return c.get_save_info(); }, constraint
    );
    to_save.push_back(std::move(info));
  }

  return to_save;
}

void save_constraints_lst(const FSys::path& path, const std::vector<constraint_set_var>& constraints_lst) {
  auto json_data = save_constraints_lst_to_json(constraints_lst);
  if (json_data.is_null()) {
    return;
  }

  std::ofstream ofs{path};
  DOODLE_CHICK(ofs.is_open(), "无法写入约束 JSON 文件: {}", path.string());
  ofs << json_data.dump(2);
  SPDLOG_INFO("Saved {} constraints to {}", constraints_lst.size(), path.string());
}

}  // namespace doodle::ai
