//
// Created by TD on 25-7-21.
//
#include "kimodo_motion_rep.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/ai/kimodo.h>
#include <doodle_lib/ai/motion_rep/geometry.h>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace doodle::ai {

kimodo_motion_rep::kimodo_motion_rep(std::shared_ptr<skeleton_base> skel, std::shared_ptr<kimodo_model_config> cfg) {
  DOODLE_CHICK(cfg, "kimodo_model_config 为空");
  skeleton_      = std::move(skel);
  fps_           = static_cast<float>(cfg->fps_);
  nbjoints_      = skeleton_->nbjoints_;

  // ---- 定义特征布局（对应 Python size_dict） ----
  feature_names_ = {"smooth_root_pos", "global_root_heading", "local_joints_positions",
                    "global_rot_data", "velocities",          "foot_contacts"};

  feature_sizes_ = {
      3,              // smooth_root_pos
      2,              // global_root_heading
      nbjoints_ * 3,  // local_joints_positions
      nbjoints_ * 6,  // global_rot_data
      nbjoints_ * 3,  // velocities
      4               // foot_contacts
  };

  last_root_feature_ = "global_root_heading";

  // 局部根维度（local_root_size_dict）
  local_root_dim_    = 1 + 2 + 1;  // local_root_rot_vel + local_root_vel + global_root_y

  // 构建切片字典
  build_slice_dict();

  // 加载统计信息
  if (!cfg->stats_path_.empty() && FSys::exists(cfg->stats_path_)) {
    load_stats(cfg->stats_path_);
    SPDLOG_INFO("kimodo_motion_rep: 从 {} 加载统计信息", cfg->stats_path_.string());
  }

  SPDLOG_INFO(
      "kimodo_motion_rep 初始化完成: nbjoints={}, motion_rep_dim={}, global_root_dim={}, body_dim={}, "
      "local_root_dim={}",
      nbjoints_, motion_rep_dim_, global_root_dim_, body_dim_, local_root_dim_
  );
}

// ======================================================================
// encode: 局部旋转 + 根位置 → 平滑根特征
// ======================================================================
MatrixXfRow kimodo_motion_rep::encode(
    const MatrixXfRow& local_joint_rots, const MatrixXfRow& root_positions, bool to_normalize, std::int64_t batch_size,
    std::int64_t time_steps, const Eigen::VectorXi& lengths
) const {
  const Eigen::Index total = local_joint_rots.rows();
  DOODLE_CHICK(total == batch_size * time_steps, "总帧数不匹配");
  DOODLE_CHICK(local_joint_rots.cols() == nbjoints_ * 9, "旋转矩阵列数不匹配 J*9");
  DOODLE_CHICK(root_positions.cols() == 3, "根位置列数 != 3");

  // 有效长度
  Eigen::VectorXi actual_lengths = lengths;
  if (actual_lengths.size() == 0) {
    actual_lengths = Eigen::VectorXi::Constant(static_cast<Eigen::Index>(batch_size), time_steps);
  }

  // ---- Step 1: FK ----
  auto fk_res               = skeleton_->fk(local_joint_rots, root_positions);
  // fk_res.global_rot_mats: [B*T, J*9]
  // fk_res.posed_joints: [B*T, J*3]
  // fk_res.posed_joints_norootpos: [B*T, J*3]

  // ---- Step 2: 计算朝向角 ----
  MatrixXfRow heading_angle = compute_heading_angle(fk_res.posed_joints, *skeleton_, batch_size, time_steps);

  // global_root_heading = [cos(angle), sin(angle)]
  MatrixXfRow global_root_heading(total, 2);
  for (Eigen::Index i = 0; i < total; ++i) {
    const Eigen::Index b      = i / time_steps;
    const Eigen::Index t      = i % time_steps;
    global_root_heading(i, 0) = std::cos(heading_angle(b, t));
    global_root_heading(i, 1) = std::sin(heading_angle(b, t));
  }

  // ---- Step 3: 平滑根位置 ----
  MatrixXfRow smooth_root_pos = get_smooth_root_pos(root_positions, batch_size, time_steps);

  // ---- Step 4: 局部关节位置（相对平滑根） ----
  // hips_offset = root_positions - smooth_root_pos
  // hips_offset.y = root_positions.y
  // local_joints_positions = posed_joints_norootpos + hips_offset[:, None]
  //   = posed_joints - root_positions + root_positions - smooth_root_pos (但保留Y为root)
  // 简化: local_joints_positions = fk_res.posed_joints - smooth_root_pos (在xz平面)
  //       并保持Y为posed_joints的Y
  MatrixXfRow local_joints_positions(total, nbjoints_ * 3);
  for (Eigen::Index i = 0; i < total; ++i) {
    for (Eigen::Index j = 0; j < nbjoints_; ++j) {
      const float px                       = fk_res.posed_joints(i, j * 3 + 0);
      const float py                       = fk_res.posed_joints(i, j * 3 + 1);
      const float pz                       = fk_res.posed_joints(i, j * 3 + 2);

      // xz 相对平滑根
      local_joints_positions(i, j * 3 + 0) = px - smooth_root_pos(i, 0);
      local_joints_positions(i, j * 3 + 1) = py;
      local_joints_positions(i, j * 3 + 2) = pz - smooth_root_pos(i, 2);
    }
  }

  // ---- Step 5: 速度 ----
  MatrixXfRow velocities =
      compute_vel_xyz(fk_res.posed_joints, fps_, batch_size, time_steps, nbjoints_, actual_lengths);

  // ---- Step 6: 脚接触检测 ----
  auto foot_contacts_bool =
      foot_detect_from_pos_and_vel(fk_res.posed_joints, velocities, *skeleton_, batch_size, time_steps);
  MatrixXfRow foot_contacts(total, 4);
  for (Eigen::Index i = 0; i < total; ++i) {
    for (int c = 0; c < 4; ++c) {
      foot_contacts(i, c) = foot_contacts_bool(i, c) ? 1.0f : 0.0f;
    }
  }

  // ---- Step 7: 全局旋转转 6D ----
  MatrixXfRow global_rot_data = matrix_to_cont6d(fk_res.global_rot_mats);

  // ---- Step 8: 拼接所有特征 ----
  MatrixXfRow features(total, motion_rep_dim_);

  // 按 label 顺序拼接
  // smooth_root_pos(3) + global_root_heading(2) + local_joints_positions(J*3)
  //   + global_rot_data(J*6) + velocities(J*3) + foot_contacts(4)
  std::int64_t offset            = 0;

  // smooth_root_pos
  features.middleCols(offset, 3) = smooth_root_pos;
  offset += 3;

  // global_root_heading
  features.middleCols(offset, 2) = global_root_heading;
  offset += 2;

  // local_joints_positions
  features.middleCols(offset, nbjoints_ * 3) = local_joints_positions;
  offset += nbjoints_ * 3;

  // global_rot_data
  features.middleCols(offset, nbjoints_ * 6) = global_rot_data;
  offset += nbjoints_ * 6;

  // velocities
  features.middleCols(offset, nbjoints_ * 3) = velocities;
  offset += nbjoints_ * 3;

  // foot_contacts
  features.middleCols(offset, 4) = foot_contacts;
  offset += 4;

  DOODLE_CHICK(offset == motion_rep_dim_, "特征拼接偏移 {} 不匹配 motion_rep_dim {}", offset, motion_rep_dim_);

  // ---- 标准化 ----
  if (to_normalize) {
    features = normalize(features);
  }

  return features;
}

// ======================================================================
// decode: 平滑根特征 → 运动输出
// ======================================================================
motion_output kimodo_motion_rep::decode(
    const MatrixXfRow& features, bool is_normalized, std::int64_t batch_size, std::int64_t time_steps
) const {
  const Eigen::Index total = features.rows();
  DOODLE_CHICK(total == batch_size * time_steps, "总帧数不匹配");
  DOODLE_CHICK(features.cols() == motion_rep_dim_, "特征维度不匹配");

  MatrixXfRow feats = features;
  if (is_normalized) {
    feats = unnormalize(feats);
  }

  // ---- 拆包 ----
  std::int64_t offset               = 0;

  const MatrixXfRow smooth_root_pos = feats.middleCols<3>(offset);
  offset += 3;

  const MatrixXfRow global_root_heading = feats.middleCols<2>(offset);
  offset += 2;

  const MatrixXfRow local_joints_positions = feats.middleCols(offset, nbjoints_ * 3);
  offset += nbjoints_ * 3;

  const MatrixXfRow global_rot_data = feats.middleCols(offset, nbjoints_ * 6);
  offset += nbjoints_ * 6;

  // velocities (跳过，解码不使用)
  offset += nbjoints_ * 3;

  const MatrixXfRow foot_contacts_float = feats.middleCols<4>(offset);
  offset += 4;

  DOODLE_CHICK(offset == motion_rep_dim_, "拆包偏移 {} 不匹配 motion_rep_dim {}", offset, motion_rep_dim_);

  // ---- 6D → 3x3 矩阵 ----
  MatrixXfRow global_rot_mats       = cont6d_to_matrix(global_rot_data);  // [B*T, J*9]

  // ---- 全局旋转 → 局部旋转 ----
  MatrixXfRow local_rot_mats        = skeleton_->global_rots_to_local_rots(global_rot_mats);

  // ---- 从局部关节位置计算根位置 ----
  // posed_joints_from_pos = local_joints_positions
  // posed_joints_from_pos.x += smooth_root_pos.x
  // posed_joints_from_pos.z += smooth_root_pos.z
  // root_positions = posed_joints_from_pos[..., root_idx, :]
  MatrixXfRow posed_joints_from_pos = local_joints_positions;
  for (Eigen::Index i = 0; i < total; ++i) {
    for (Eigen::Index j = 0; j < nbjoints_; ++j) {
      posed_joints_from_pos(i, j * 3 + 0) += smooth_root_pos(i, 0);
      posed_joints_from_pos(i, j * 3 + 2) += smooth_root_pos(i, 2);
    }
  }

  MatrixXfRow root_positions(total, 3);
  for (Eigen::Index i = 0; i < total; ++i) {
    root_positions(i, 0) = posed_joints_from_pos(i, skeleton_->root_idx_ * 3 + 0);
    root_positions(i, 1) = posed_joints_from_pos(i, skeleton_->root_idx_ * 3 + 1);
    root_positions(i, 2) = posed_joints_from_pos(i, skeleton_->root_idx_ * 3 + 2);
  }

  // ---- FK 计算全局关节位置 ----
  auto fk_res = skeleton_->fk(local_rot_mats, root_positions);

  // ---- 脚接触二值化 ----
  MatrixXbRow foot_contacts_bool(total, 4);
  for (Eigen::Index i = 0; i < total; ++i) {
    for (int c = 0; c < 4; ++c) {
      foot_contacts_bool(i, c) = foot_contacts_float(i, c) > 0.5f;
    }
  }

  // ---- 构建输出 ----
  motion_output out;
  out.local_rot_mats      = std::move(local_rot_mats);
  out.global_rot_mats     = std::move(global_rot_mats);
  out.posed_joints        = std::move(fk_res.posed_joints);
  out.root_positions      = std::move(root_positions);
  out.smooth_root_pos     = smooth_root_pos;
  out.foot_contacts       = std::move(foot_contacts_bool);
  out.global_root_heading = global_root_heading;

  return out;
}

// ======================================================================
// rotate: 按朝向角旋转特征
// ======================================================================
MatrixXfRow kimodo_motion_rep::rotate(
    const MatrixXfRow& features, const Eigen::VectorXf& angle, std::int64_t batch_size, std::int64_t time_steps
) const {
  const Eigen::Index total = features.rows();
  DOODLE_CHICK(total == batch_size * time_steps, "总帧数不匹配");
  DOODLE_CHICK(features.cols() == motion_rep_dim_, "特征维度不匹配");
  DOODLE_CHICK(angle.size() == batch_size, "角度数不匹配 batch_size");

  // 拆包特征块
  const std::int64_t s1        = 3;              // smooth_root_pos
  const std::int64_t s2        = 2;              // global_root_heading
  const std::int64_t s3        = nbjoints_ * 3;  // local_joints_positions
  const std::int64_t s4        = nbjoints_ * 6;  // global_rot_data
  const std::int64_t s5        = nbjoints_ * 3;  // velocities
  const std::int64_t s6        = 4;              // foot_contacts

  MatrixXfRow smooth_root_pos  = features.middleCols(0, s1);
  MatrixXfRow heading_2d       = features.middleCols(s1, s2);
  MatrixXfRow local_joints_pos = features.middleCols(s1 + s2, s3);
  MatrixXfRow global_rot_6d    = features.middleCols(s1 + s2 + s3, s4);
  MatrixXfRow velocities       = features.middleCols(s1 + s2 + s3 + s4, s5);
  // foot_contacts 不旋转

  rotate_features rf(angle);

  MatrixXfRow new_smooth_root = rf.rotate_positions(smooth_root_pos, batch_size, time_steps);
  MatrixXfRow new_heading     = rf.rotate_2d_positions(heading_2d, batch_size, time_steps);
  MatrixXfRow new_joints      = rf.rotate_positions(local_joints_pos, batch_size, time_steps);
  MatrixXfRow new_rot_6d      = rf.rotate_6d_rotations(global_rot_6d, batch_size, time_steps, nbjoints_);
  MatrixXfRow new_vel         = rf.rotate_positions(velocities, batch_size, time_steps);

  // 拼接
  MatrixXfRow result(total, motion_rep_dim_);
  std::int64_t offset           = 0;
  result.middleCols(offset, s1) = new_smooth_root;
  offset += s1;
  result.middleCols(offset, s2) = new_heading;
  offset += s2;
  result.middleCols(offset, s3) = new_joints;
  offset += s3;
  result.middleCols(offset, s4) = new_rot_6d;
  offset += s4;
  result.middleCols(offset, s5) = new_vel;
  offset += s5;
  result.middleCols(offset, s6) = features.middleCols(offset, s6);  // foot_contacts 原样

  return result;
}

// ======================================================================
// translate_2d: 平移 XZ 平面
// ======================================================================
MatrixXfRow kimodo_motion_rep::translate_2d(
    const MatrixXfRow& features, const MatrixXfRow& translation_2d, std::int64_t batch_size, std::int64_t time_steps
) const {
  const Eigen::Index total = features.rows();
  DOODLE_CHICK(total == batch_size * time_steps, "总帧数不匹配");
  DOODLE_CHICK(features.cols() == motion_rep_dim_, "特征维度不匹配");
  DOODLE_CHICK(translation_2d.rows() == batch_size, "平移行数不匹配");
  DOODLE_CHICK(translation_2d.cols() == 2, "平移列数 != 2");

  MatrixXfRow result = features;

  // smooth_root_pos 在列 [0, 3)
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    const float dx = translation_2d(b, 0);
    const float dz = translation_2d(b, 1);

    for (Eigen::Index t = 0; t < time_steps; ++t) {
      const Eigen::Index idx = b * time_steps + t;
      result(idx, 0) += dx;  // x
      result(idx, 2) += dz;  // z
    }
  }

  return result;
}

// ======================================================================
// create_conditions: 从约束构建条件和掩码
// ======================================================================
kimodo_motion_rep::condition_result kimodo_motion_rep::create_conditions(
    const std::unordered_map<std::string, std::vector<MatrixXfRow>>& index_dict,
    const std::unordered_map<std::string, std::vector<MatrixXfRow>>& data_dict, std::int64_t length, bool to_normalize
) const {
  const std::int64_t D = motion_rep_dim_;

  condition_result result;
  result.observed_motion = MatrixXfRow::Zero(length, D);
  result.motion_mask     = MatrixXbRow::Zero(length, D);

  // 处理 smooth_root_2d 约束
  auto idx_it            = index_dict.find("smooth_root_2d");
  if (idx_it != index_dict.end() && !idx_it->second.empty()) {
    // 合并所有索引
    std::vector<Eigen::Index> all_indices;
    for (const auto& mat : idx_it->second) {
      for (Eigen::Index r = 0; r < mat.rows(); ++r) {
        all_indices.push_back(static_cast<Eigen::Index>(mat(r, 0)));
      }
    }
    // 去重
    std::sort(all_indices.begin(), all_indices.end());
    all_indices.erase(std::unique(all_indices.begin(), all_indices.end()), all_indices.end());

    // 合并所有数据
    std::vector<float> all_data_x, all_data_z;
    auto data_it = data_dict.find("smooth_root_2d");
    if (data_it != data_dict.end()) {
      for (const auto& mat : data_it->second) {
        for (Eigen::Index r = 0; r < mat.rows(); ++r) {
          all_data_x.push_back(mat(r, 0));
          all_data_z.push_back(mat(r, 1));
        }
      }
    }

    const std::int64_t smooth_start = feature_start_.at("smooth_root_pos");
    for (std::size_t i = 0; i < all_indices.size() && i < all_data_x.size(); ++i) {
      const Eigen::Index t = all_indices[i];
      if (t < length) {
        result.observed_motion(t, smooth_start + 0) = all_data_x[i];
        result.observed_motion(t, smooth_start + 2) = all_data_z[i];
        result.motion_mask(t, smooth_start + 0)     = true;
        result.motion_mask(t, smooth_start + 2)     = true;
      }
    }
  }

  // 处理 root_y_pos 约束
  idx_it = index_dict.find("root_y_pos");
  if (idx_it != index_dict.end() && !idx_it->second.empty()) {
    std::vector<Eigen::Index> all_indices;
    for (const auto& mat : idx_it->second) {
      for (Eigen::Index r = 0; r < mat.rows(); ++r) {
        all_indices.push_back(static_cast<Eigen::Index>(mat(r, 0)));
      }
    }
    std::sort(all_indices.begin(), all_indices.end());
    all_indices.erase(std::unique(all_indices.begin(), all_indices.end()), all_indices.end());

    std::vector<float> all_data_y;
    auto data_it = data_dict.find("root_y_pos");
    if (data_it != data_dict.end()) {
      for (const auto& mat : data_it->second) {
        for (Eigen::Index r = 0; r < mat.rows(); ++r) {
          all_data_y.push_back(mat(r, 0));
        }
      }
    }

    const std::int64_t smooth_start = feature_start_.at("smooth_root_pos");
    for (std::size_t i = 0; i < all_indices.size() && i < all_data_y.size(); ++i) {
      const Eigen::Index t = all_indices[i];
      if (t < length) {
        result.observed_motion(t, smooth_start + 1) = all_data_y[i];
        result.motion_mask(t, smooth_start + 1)     = true;
      }
    }
  }

  // 处理 global_root_heading 约束
  idx_it = index_dict.find("global_root_heading");
  if (idx_it != index_dict.end() && !idx_it->second.empty()) {
    std::vector<Eigen::Index> all_indices;
    for (const auto& mat : idx_it->second) {
      for (Eigen::Index r = 0; r < mat.rows(); ++r) {
        all_indices.push_back(static_cast<Eigen::Index>(mat(r, 0)));
      }
    }
    std::sort(all_indices.begin(), all_indices.end());
    all_indices.erase(std::unique(all_indices.begin(), all_indices.end()), all_indices.end());

    std::vector<float> all_cos, all_sin;
    auto data_it = data_dict.find("global_root_heading");
    if (data_it != data_dict.end()) {
      for (const auto& mat : data_it->second) {
        for (Eigen::Index r = 0; r < mat.rows(); ++r) {
          all_cos.push_back(mat(r, 0));
          all_sin.push_back(mat(r, 1));
        }
      }
    }

    const std::int64_t heading_start = feature_start_.at("global_root_heading");
    for (std::size_t i = 0; i < all_indices.size() && i < all_cos.size(); ++i) {
      const Eigen::Index t = all_indices[i];
      if (t < length) {
        result.observed_motion(t, heading_start + 0) = all_cos[i];
        result.observed_motion(t, heading_start + 1) = all_sin[i];
        result.motion_mask(t, heading_start + 0)     = true;
        result.motion_mask(t, heading_start + 1)     = true;
      }
    }
  }

  // 处理 global_joints_rots 约束
  idx_it = index_dict.find("global_joints_rots");
  if (idx_it != index_dict.end() && !idx_it->second.empty()) {
    const std::int64_t rot_start = feature_start_.at("global_rot_data");

    // 简化的处理：假设 index_dict 每个元素是 [frame_idx, joint_idx]
    for (const auto& mat : idx_it->second) {
      for (Eigen::Index r = 0; r < mat.rows(); ++r) {
        const Eigen::Index t_idx = static_cast<Eigen::Index>(mat(r, 0));
        const Eigen::Index j_idx = static_cast<Eigen::Index>(mat(r, 1));
        if (t_idx < length && j_idx < nbjoints_) {
          result.motion_mask(t_idx, rot_start + j_idx * 6 + 0) = true;
          result.motion_mask(t_idx, rot_start + j_idx * 6 + 1) = true;
          result.motion_mask(t_idx, rot_start + j_idx * 6 + 2) = true;
          result.motion_mask(t_idx, rot_start + j_idx * 6 + 3) = true;
          result.motion_mask(t_idx, rot_start + j_idx * 6 + 4) = true;
          result.motion_mask(t_idx, rot_start + j_idx * 6 + 5) = true;
        }
      }
    }

    // 填充数据
    auto data_it = data_dict.find("global_joints_rots");
    if (data_it != data_dict.end()) {
      std::size_t data_idx = 0;
      for (const auto& mat : data_it->second) {
        for (Eigen::Index r = 0; r < mat.rows(); ++r) {
          if (data_idx < idx_it->second.size()) {
            // 将旋转矩阵转 6D
            Eigen::Vector3f x_raw(mat(r, 0), mat(r, 1), mat(r, 2));
            Eigen::Vector3f y_raw(mat(r, 3), mat(r, 4), mat(r, 5));
            Eigen::Vector3f x        = x_raw.normalized();
            Eigen::Vector3f z        = x.cross(y_raw).normalized();
            Eigen::Vector3f y        = z.cross(x);

            const Eigen::Index t_idx = static_cast<Eigen::Index>(idx_it->second[data_idx](r, 0));
            const Eigen::Index j_idx = static_cast<Eigen::Index>(idx_it->second[data_idx](r, 1));
            if (t_idx < length && j_idx < nbjoints_) {
              result.observed_motion(t_idx, rot_start + j_idx * 6 + 0) = x(0);
              result.observed_motion(t_idx, rot_start + j_idx * 6 + 1) = x(1);
              result.observed_motion(t_idx, rot_start + j_idx * 6 + 2) = x(2);
              result.observed_motion(t_idx, rot_start + j_idx * 6 + 3) = y(0);
              result.observed_motion(t_idx, rot_start + j_idx * 6 + 4) = y(1);
              result.observed_motion(t_idx, rot_start + j_idx * 6 + 5) = y(2);
            }
          }
          ++data_idx;
        }
      }
    }
  }

  // 标准化
  if (to_normalize) {
    result.observed_motion = normalize(result.observed_motion);
  }

  return result;
}

// ======================================================================
// create_conditions_from_constraints_batched: 批量创建条件
// ======================================================================
kimodo_motion_rep::batched_condition_result kimodo_motion_rep::create_conditions_from_constraints_batched(
    const std::vector<std::vector<std::pair<std::string, std::vector<MatrixXfRow>>>>& constraints_lst,
    const Eigen::VectorXi& lengths, bool to_normalize
) const {
  const Eigen::Index B       = lengths.size();
  const std::int64_t max_len = lengths.maxCoeff();
  const std::int64_t D       = motion_rep_dim_;

  batched_condition_result result;
  result.observed_motion = MatrixXfRow::Zero(B * max_len, D);
  result.motion_mask     = MatrixXbRow::Zero(B * max_len, D);

  if (constraints_lst.empty()) {
    return result;
  }

  // 对每个样本分别处理
  for (Eigen::Index b = 0; b < B; ++b) {
    // 当前样本的约束
    for (const auto& constraint_pair : constraints_lst[static_cast<std::size_t>(b)]) {
      const auto& type     = constraint_pair.first;
      const auto& data_vec = constraint_pair.second;

      if (type == "FullBodyConstraintSet" || type == "EndEffectorConstraintSet") {
        // 简化的约束处理：提取 smooth_root_2d、global_root_heading、global_joints_rots
        // 实际完整实现需要解析约束对象的内部结构
        if (data_vec.size() >= 3) {
          // data_vec[0]: frame_indices [K, 1]
          // data_vec[1]: smooth_root_2d [K, 2]
          // data_vec[2]: root_heading [K, 2]
          const auto& frame_idx            = data_vec[0];
          const auto& root_2d              = data_vec[1];
          const auto& heading              = data_vec[2];

          const std::int64_t smooth_start  = feature_start_.at("smooth_root_pos");
          const std::int64_t heading_start = feature_start_.at("global_root_heading");

          for (Eigen::Index r = 0; r < frame_idx.rows() && r < root_2d.rows(); ++r) {
            const Eigen::Index t = b * max_len + static_cast<Eigen::Index>(frame_idx(r, 0));
            if (t < b * max_len + lengths(b)) {
              result.observed_motion(t, smooth_start + 0) = root_2d(r, 0);
              result.observed_motion(t, smooth_start + 2) = root_2d(r, 1);
              result.motion_mask(t, smooth_start + 0)     = true;
              result.motion_mask(t, smooth_start + 2)     = true;
            }
          }

          for (Eigen::Index r = 0; r < frame_idx.rows() && r < heading.rows(); ++r) {
            const Eigen::Index t = b * max_len + static_cast<Eigen::Index>(frame_idx(r, 0));
            if (t < b * max_len + lengths(b)) {
              result.observed_motion(t, heading_start + 0) = heading(r, 0);
              result.observed_motion(t, heading_start + 1) = heading(r, 1);
              result.motion_mask(t, heading_start + 0)     = true;
              result.motion_mask(t, heading_start + 1)     = true;
            }
          }
        }
      }
    }
  }

  if (to_normalize) {
    result.observed_motion = normalize(result.observed_motion);
  }

  return result;
}

}  // namespace doodle::ai
