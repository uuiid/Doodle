//
// Created by TD on 25-7-21.
//
#include "motion_rep_base.h"

#include <doodle_core/exception/exception.h>

#include "geometry.h"
#include <fmt/format.h>
#include <numeric>

namespace doodle::ai {

// ======================================================================
// motion_stats
// ======================================================================
void motion_stats::load(const FSys::path& folder) {
  const auto mean_path = folder / "mean.npy";
  const auto std_path  = folder / "std.npy";

  DOODLE_CHICK(FSys::exists(mean_path), "mean.npy 不存在: {}", mean_path.string());
  DOODLE_CHICK(FSys::exists(std_path), "std.npy 不存在: {}", std_path.string());

  // TODO: 实现 .npy 文件解析或使用现有工具
  // 暂时留空，由上层调用 set_from_vectors() 设置
  SPDLOG_WARN("motion_stats::load() 尚未实现 .npy 解析，请使用 set_from_vectors()");
}

Eigen::MatrixXf motion_stats::normalize(const Eigen::MatrixXf& data) const {
  DOODLE_CHICK(is_valid(), "统计信息未初始化");
  DOODLE_CHICK(data.cols() == dim(), "数据列数 {} 不匹配统计维度 {}", data.cols(), dim());

  const auto mean_arr = mean.transpose().replicate(data.rows(), 1);
  const auto std_arr  = std.transpose().replicate(data.rows(), 1);

  return (data.array() - mean_arr.array()) / (std_arr.array().square() + eps).sqrt();
}

Eigen::MatrixXf motion_stats::unnormalize(const Eigen::MatrixXf& data) const {
  DOODLE_CHICK(is_valid(), "统计信息未初始化");
  DOODLE_CHICK(data.cols() == dim(), "数据列数 {} 不匹配统计维度 {}", data.cols(), dim());

  const auto mean_arr = mean.transpose().replicate(data.rows(), 1);
  const auto std_arr  = std.transpose().replicate(data.rows(), 1);

  return data.array() * (std_arr.array().square() + eps).sqrt() + mean_arr.array();
}

// ======================================================================
// motion_rep_base
// ======================================================================
void motion_rep_base::build_slice_dict() {
  feature_start_.clear();
  feature_end_.clear();

  std::int64_t offset = 0;
  for (std::size_t i = 0; i < feature_names_.size(); ++i) {
    const auto& name     = feature_names_[i];
    feature_start_[name] = offset;
    offset += feature_sizes_[i];
    feature_end_[name] = offset;
  }

  motion_rep_dim_ = offset;

  // 计算根切片和身体切片
  if (!last_root_feature_.empty() && feature_end_.count(last_root_feature_)) {
    global_root_dim_ = feature_end_.at(last_root_feature_);
  } else {
    // 默认根切片只包含第一个特征
    global_root_dim_ = feature_sizes_.empty() ? 0 : feature_sizes_[0];
  }

  body_dim_ = motion_rep_dim_ - global_root_dim_;
}

void motion_rep_base::load_stats(const FSys::path& stats_path) {
  const auto gr_path = stats_path / "global_root";
  const auto lr_path = stats_path / "local_root";
  const auto b_path  = stats_path / "body";

  DOODLE_CHICK(FSys::exists(gr_path), "global_root stats 目录不存在: {}", gr_path.string());
  DOODLE_CHICK(FSys::exists(lr_path), "local_root stats 目录不存在: {}", lr_path.string());
  DOODLE_CHICK(FSys::exists(b_path), "body stats 目录不存在: {}", b_path.string());

  global_root_stats_.load(gr_path);
  local_root_stats_.load(lr_path);
  body_stats_.load(b_path);

  // 合并统计（global_root + body）
  if (global_root_stats_.is_valid() && body_stats_.is_valid()) {
    Eigen::VectorXf combined_mean(global_root_stats_.dim() + body_stats_.dim());
    Eigen::VectorXf combined_std(global_root_stats_.dim() + body_stats_.dim());

    combined_mean.head(global_root_stats_.dim()) = global_root_stats_.mean;
    combined_mean.tail(body_stats_.dim())        = body_stats_.mean;

    combined_std.head(global_root_stats_.dim())  = global_root_stats_.std;
    combined_std.tail(body_stats_.dim())         = body_stats_.std;

    combined_stats_.set_from_vectors(combined_mean, combined_std);
  }
}

std::int64_t motion_rep_base::feature_size(const std::string& name) const {
  const auto it = feature_end_.find(name);
  if (it == feature_end_.end()) return 0;
  return it->second - feature_start_.at(name);
}

Eigen::MatrixXf motion_rep_base::normalize(const Eigen::MatrixXf& features) const {
  return combined_stats_.normalize(features);
}

Eigen::MatrixXf motion_rep_base::unnormalize(const Eigen::MatrixXf& features) const {
  return combined_stats_.unnormalize(features);
}

Eigen::MatrixXf motion_rep_base::get_root_pos(
    const Eigen::MatrixXf& features, std::int64_t batch_size, std::int64_t time_steps
) const {
  // 从 smooth_root_pos 提取根位置
  // 需要子类定义中包含 "smooth_root_pos"
  const auto sit = feature_start_.find("smooth_root_pos");
  DOODLE_CHICK(sit != feature_start_.end(), "特征不含 smooth_root_pos  或 root_pos");
  
  const std::int64_t start = sit->second;
  const std::int64_t end   = feature_end_.at("smooth_root_pos");
  return features.middleCols(start, end - start);
}

Eigen::MatrixXf motion_rep_base::global_root_to_local_root(
    const Eigen::MatrixXf& root_features, bool normalized, std::int64_t batch_size, std::int64_t time_steps,
    const Eigen::VectorXi& lengths
) const {
  // root_features: [B*T, global_root_dim] = [smooth_root_pos(3) + global_root_heading(2)]
  DOODLE_CHICK(
      root_features.cols() == global_root_dim_, "根特征列数 {} 不匹配 {}", root_features.cols(), global_root_dim_
  );

  Eigen::MatrixXf rf;
  if (normalized) {
    rf = global_root_stats_.unnormalize(root_features);
  } else {
    rf = root_features;
  }

  // 拆包：smooth_root_pos [B*T, 3], global_root_heading [B*T, 2]
  const Eigen::MatrixXf root_pos   = rf.leftCols(3);
  const Eigen::MatrixXf heading_2d = rf.middleCols(3, 2);

  // 将 cos/sin 转为角度
  Eigen::MatrixXf heading_angle(batch_size, time_steps);
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    for (Eigen::Index t = 0; t < time_steps; ++t) {
      const Eigen::Index idx = b * time_steps + t;
      heading_angle(b, t)    = std::atan2(heading_2d(idx, 1), heading_2d(idx, 0));
    }
  }

  // local_root_rot_vel: angular velocity of heading
  Eigen::MatrixXf local_root_rot_vel = compute_vel_angle(heading_angle, fps_, lengths);
  // local_root_vel: 2D planar velocity of root in xz plane
  const Eigen::Index J               = 1;         // treat root as a single joint
  Eigen::MatrixXf root_pos_3d        = root_pos;  // [B*T, 3]
  // 需要 expand 维度为 [B*T, 1, 3]
  Eigen::MatrixXf root_pos_expanded(batch_size * time_steps, 3);
  root_pos_expanded       = root_pos;
  // 用 compute_vel_xyz 计算
  Eigen::MatrixXf vel_xyz = compute_vel_xyz(root_pos_expanded, fps_, batch_size, time_steps, 1, lengths);
  // 取 xz 分量: [B*T, 2]
  Eigen::MatrixXf local_root_vel(batch_size * time_steps, 2);
  for (Eigen::Index i = 0; i < batch_size * time_steps; ++i) {
    local_root_vel(i, 0) = vel_xyz(i, 0);  // x
    local_root_vel(i, 1) = vel_xyz(i, 2);  // z
  }

  // global_root_y: height
  Eigen::MatrixXf global_root_y = root_pos.col(1);  // [B*T, 1]

  // 拼接: [local_root_rot_vel(1), local_root_vel(2), global_root_y(1)]
  // local_root_rot_vel 是 [B, T], 需要展平为 [B*T, 1]
  Eigen::MatrixXf local_root_motion(batch_size * time_steps, local_root_dim_);

  for (Eigen::Index b = 0; b < batch_size; ++b) {
    for (Eigen::Index t = 0; t < time_steps; ++t) {
      const Eigen::Index idx    = b * time_steps + t;
      local_root_motion(idx, 0) = local_root_rot_vel(b, t);  // rot_vel
      local_root_motion(idx, 1) = local_root_vel(idx, 0);    // vel_x
      local_root_motion(idx, 2) = local_root_vel(idx, 1);    // vel_z
      local_root_motion(idx, 3) = global_root_y(idx);        // height
    }
  }

  // 标准化
  if (normalized && local_root_stats_.is_valid()) {
    local_root_motion = local_root_stats_.normalize(local_root_motion);
  }

  return local_root_motion;
}

Eigen::MatrixXf motion_rep_base::get_root_heading_angle(
    const Eigen::MatrixXf& features, std::int64_t batch_size, std::int64_t time_steps
) const {
  const auto sit = feature_start_.find("global_root_heading");
  DOODLE_CHICK(sit != feature_start_.end(), "特征不含 global_root_heading");

  const std::int64_t start         = sit->second;
  const Eigen::MatrixXf heading_2d = features.middleCols(start, 2);

  Eigen::MatrixXf angle(batch_size, time_steps);
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    for (Eigen::Index t = 0; t < time_steps; ++t) {
      const Eigen::Index idx = b * time_steps + t;
      angle(b, t)            = std::atan2(heading_2d(idx, 1), heading_2d(idx, 0));
    }
  }
  return angle;
}

Eigen::MatrixXf motion_rep_base::rotate_to(
    const Eigen::MatrixXf& features, const Eigen::VectorXf& target_angle, std::int64_t batch_size,
    std::int64_t time_steps
) const {
  // 计算当前第 0 帧的朝向角
  Eigen::MatrixXf current_angle = get_root_heading_angle(features, batch_size, time_steps);
  Eigen::VectorXf delta_angle(batch_size);
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    delta_angle(b) = target_angle(b) - current_angle(b, 0);
  }
  return rotate(features, delta_angle, batch_size, time_steps);
}

Eigen::MatrixXf motion_rep_base::rotate_to_zero(
    const Eigen::MatrixXf& features, std::int64_t batch_size, std::int64_t time_steps
) const {
  Eigen::VectorXf zero_angle = Eigen::VectorXf::Zero(batch_size);
  return rotate_to(features, zero_angle, batch_size, time_steps);
}

Eigen::MatrixXf motion_rep_base::translate_2d_to(
    const Eigen::MatrixXf& features, const Eigen::MatrixXf& target_2d_pos, std::int64_t batch_size,
    std::int64_t time_steps
) const {
  // 获取当前第 0 帧的根位置 (xz)
  Eigen::MatrixXf root_pos = get_root_pos(features, batch_size, time_steps);

  Eigen::MatrixXf delta_pos(batch_size, 2);
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    const Eigen::Index idx = b * time_steps;
    delta_pos(b, 0)        = target_2d_pos(b, 0) - root_pos(idx, 0);  // dx
    delta_pos(b, 1)        = target_2d_pos(b, 1) - root_pos(idx, 2);  // dz
  }

  return translate_2d(features, delta_pos, batch_size, time_steps);
}

Eigen::MatrixXf motion_rep_base::translate_2d_to_zero(
    const Eigen::MatrixXf& features, std::int64_t batch_size, std::int64_t time_steps
) const {
  Eigen::MatrixXf zero_pos = Eigen::MatrixXf::Zero(batch_size, 2);
  return translate_2d_to(features, zero_pos, batch_size, time_steps);
}

Eigen::MatrixXf motion_rep_base::canonicalize(
    const Eigen::MatrixXf& features, bool normalized, std::int64_t batch_size, std::int64_t time_steps
) const {
  Eigen::MatrixXf feats = features;
  if (normalized) {
    feats = unnormalize(feats);
  }
  feats = rotate_to_zero(feats, batch_size, time_steps);
  feats = translate_2d_to_zero(feats, batch_size, time_steps);
  if (normalized) {
    feats = normalize(feats);
  }
  return feats;
}

}  // namespace doodle::ai
