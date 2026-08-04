//
// Created by TD on 25-7-21.
//
#include "motion_rep_base.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/ai/motion_rep/geometry.h>

#include <cnpy.h>
#include <fmt/format.h>

namespace doodle::ai {

// ======================================================================
// motion_stats
// ======================================================================
void motion_stats::load(const FSys::path& folder) {
  const auto mean_path = folder / "mean.npy";
  const auto std_path  = folder / "std.npy";

  DOODLE_CHICK(FSys::exists(mean_path), "mean.npy 不存在: {}", mean_path.string());
  DOODLE_CHICK(FSys::exists(std_path), "std.npy 不存在: {}", std_path.string());

  // 加载 mean.npy（应为 1D 向量）
  auto mean_data = cnpy::npy_load(mean_path.string());
  // 检查加载的数据类型是否是 double
  DOODLE_CHICK(
      mean_data.word_size == sizeof(std::double_t), "mean.npy 数据类型应为 float64，当前字节大小: {}",
      mean_data.word_size
  );

  DOODLE_CHICK(mean_data.shape.size() == 1, "mean.npy shape 应为 1D，当前维度数: {}", mean_data.shape.size());
  Eigen::Map<Eigen::VectorXd> mean_map(mean_data.data<std::double_t>(), static_cast<Eigen::Index>(mean_data.shape[0]));

  // 加载 std.npy（应为 1D 向量）
  auto std_data = cnpy::npy_load(std_path.string());
  DOODLE_CHICK(
      std_data.word_size == sizeof(std::double_t), "std.npy 数据类型应为 float64，当前字节大小: {}", std_data.word_size
  );
  DOODLE_CHICK(std_data.shape.size() == 1, "std.npy shape 应为 1D，当前维度数: {}", std_data.shape.size());
  DOODLE_CHICK(
      std_data.shape[0] == mean_data.shape[0], "mean.npy 长度 {} 与 std.npy 长度 {} 不匹配", mean_data.shape[0],
      std_data.shape[0]
  );
  Eigen::Map<Eigen::VectorXd> std_map(std_data.data<std::double_t>(), static_cast<Eigen::Index>(std_data.shape[0]));

  // 拷贝到成员变量
  mean = mean_map;
  std  = std_map;

  SPDLOG_INFO("motion_stats 加载完成: dim={}", dim());
}

MatrixXfRow motion_stats::normalize(const MatrixXfRow& data) const {
  DOODLE_CHICK(is_valid(), "统计信息未初始化");
  DOODLE_CHICK(data.cols() == dim(), "数据列数 {} 不匹配统计维度 {}", data.cols(), dim());

  const auto mean_arr = mean.transpose().replicate(data.rows(), 1);
  const auto std_arr  = std.transpose().replicate(data.rows(), 1);

  return ((data.cast<std::double_t>().array() - mean_arr.array()) / (std_arr.array().square() + eps).sqrt())
      .cast<std::float_t>();
}

MatrixXfRow motion_stats::unnormalize(const MatrixXfRow& data) const {
  DOODLE_CHICK(is_valid(), "统计信息未初始化");
  DOODLE_CHICK(data.cols() == dim(), "数据列数 {} 不匹配统计维度 {}", data.cols(), dim());

  const auto mean_arr = mean.transpose().replicate(data.rows(), 1);
  const auto std_arr  = std.transpose().replicate(data.rows(), 1);

  return (data.cast<std::double_t>().array() * (std_arr.array().square() + eps).sqrt() + mean_arr.array())
      .cast<std::float_t>();
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
  DOODLE_CHICK(!feature_names_.empty(), "特征列表为空");
  DOODLE_CHICK(!feature_sizes_.empty(), "特征大小列表为空");
  DOODLE_CHICK(!last_root_feature_.empty(), "last_root_feature_ 为空");
  DOODLE_CHICK(
      feature_start_.contains(last_root_feature_), "last_root_feature_ {} 不在 feature_names_ 中", last_root_feature_
  );
  global_root_dim_ = feature_end_.at(last_root_feature_);
  body_dim_        = motion_rep_dim_ - global_root_dim_;
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
  DOODLE_CHICK(global_root_stats_.is_valid(), "global_root_stats_ 未初始化");
  DOODLE_CHICK(body_stats_.is_valid(), "body_stats_ 未初始化");
  Eigen::VectorXd combined_mean(global_root_stats_.dim() + body_stats_.dim());
  Eigen::VectorXd combined_std(global_root_stats_.dim() + body_stats_.dim());

  combined_mean.head(global_root_stats_.dim()) = global_root_stats_.mean;
  combined_mean.tail(body_stats_.dim())        = body_stats_.mean;

  combined_std.head(global_root_stats_.dim())  = global_root_stats_.std;
  combined_std.tail(body_stats_.dim())         = body_stats_.std;

  combined_stats_.set_from_vectors(combined_mean, combined_std);
}

std::int64_t motion_rep_base::feature_size(const std::string& name) const {
  const auto it = feature_end_.find(name);
  if (it == feature_end_.end()) return 0;
  return it->second - feature_start_.at(name);
}

MatrixXfRow motion_rep_base::normalize(const MatrixXfRow& features) const {
  return combined_stats_.normalize(features);
}

MatrixXfRow motion_rep_base::unnormalize(const MatrixXfRow& features) const {
  return combined_stats_.unnormalize(features);
}

MatrixXfRow motion_rep_base::get_root_pos(
    const MatrixXfRow& features, std::int64_t batch_size, std::int64_t time_steps
) const {
  // 从 smooth_root_pos 提取根位置
  // 需要子类定义中包含 "smooth_root_pos"
  const auto sit = feature_start_.find("smooth_root_pos");
  DOODLE_CHICK(sit != feature_start_.end(), "特征不含 smooth_root_pos  或 root_pos");

  const std::int64_t start = sit->second;
  const std::int64_t end   = feature_end_.at("smooth_root_pos");
  return features.middleCols(start, end - start);
}

MatrixXfRow motion_rep_base::global_root_to_local_root(
    const MatrixXfRow& root_features, bool normalized, std::int64_t batch_size, std::int64_t time_steps,
    const Eigen::VectorXi& lengths
) const {
  // root_features: [B*T, global_root_dim] = [smooth_root_pos(3) + global_root_heading(2)]
  DOODLE_CHICK(
      root_features.cols() == global_root_dim_, "根特征列数 {} 不匹配 {}", root_features.cols(), global_root_dim_
  );

  MatrixXfRow rf;
  if (normalized) {
    rf = global_root_stats_.unnormalize(root_features);
  } else {
    rf = root_features;
  }

  // 拆包：smooth_root_pos [B*T, 3], global_root_heading [B*T, 2]
  const MatrixXfRow root_pos   = rf.leftCols(3);
  const MatrixXfRow heading_2d = rf.middleCols(3, 2);

  // 将 cos/sin 转为角度
  MatrixXfRow heading_angle(batch_size, time_steps);
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    for (Eigen::Index t = 0; t < time_steps; ++t) {
      const Eigen::Index idx = b * time_steps + t;
      heading_angle(b, t)    = std::atan2(heading_2d(idx, 1), heading_2d(idx, 0));
    }
  }

  // local_root_rot_vel: angular velocity of heading
  MatrixXfRow local_root_rot_vel = compute_vel_angle(heading_angle, fps_, lengths);
  // local_root_vel: 2D planar velocity of root in xz plane
  const Eigen::Index J               = 1;         // treat root as a single joint
  MatrixXfRow root_pos_3d        = root_pos;  // [B*T, 3]
  // 需要 expand 维度为 [B*T, 1, 3]
  MatrixXfRow root_pos_expanded(batch_size * time_steps, 3);
  root_pos_expanded       = root_pos;
  // 用 compute_vel_xyz 计算
  MatrixXfRow vel_xyz = compute_vel_xyz(root_pos_expanded, fps_, batch_size, time_steps, 1, lengths);
  // 取 xz 分量: [B*T, 2]
  MatrixXfRow local_root_vel(batch_size * time_steps, 2);
  for (Eigen::Index i = 0; i < batch_size * time_steps; ++i) {
    local_root_vel(i, 0) = vel_xyz(i, 0);  // x
    local_root_vel(i, 1) = vel_xyz(i, 2);  // z
  }

  // global_root_y: height
  MatrixXfRow global_root_y = root_pos.col(1);  // [B*T, 1]

  // 拼接: [local_root_rot_vel(1), local_root_vel(2), global_root_y(1)]
  // local_root_rot_vel 是 [B, T], 需要展平为 [B*T, 1]
  MatrixXfRow local_root_motion(batch_size * time_steps, local_root_dim_);

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

MatrixXfRow motion_rep_base::get_root_heading_angle(
    const MatrixXfRow& features, std::int64_t batch_size, std::int64_t time_steps
) const {
  const auto sit = feature_start_.find("global_root_heading");
  DOODLE_CHICK(sit != feature_start_.end(), "特征不含 global_root_heading");

  const std::int64_t start         = sit->second;
  const MatrixXfRow heading_2d = features.middleCols(start, 2);

  MatrixXfRow angle(batch_size, time_steps);
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    for (Eigen::Index t = 0; t < time_steps; ++t) {
      const Eigen::Index idx = b * time_steps + t;
      angle(b, t)            = std::atan2(heading_2d(idx, 1), heading_2d(idx, 0));
    }
  }
  return angle;
}

MatrixXfRow motion_rep_base::rotate_to(
    const MatrixXfRow& features, const Eigen::VectorXf& target_angle, std::int64_t batch_size,
    std::int64_t time_steps
) const {
  // 计算当前第 0 帧的朝向角
  MatrixXfRow current_angle = get_root_heading_angle(features, batch_size, time_steps);
  Eigen::VectorXf delta_angle(batch_size);
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    delta_angle(b) = target_angle(b) - current_angle(b, 0);
  }
  return rotate(features, delta_angle, batch_size, time_steps);
}

MatrixXfRow motion_rep_base::rotate_to_zero(
    const MatrixXfRow& features, std::int64_t batch_size, std::int64_t time_steps
) const {
  Eigen::VectorXf zero_angle = Eigen::VectorXf::Zero(batch_size);
  return rotate_to(features, zero_angle, batch_size, time_steps);
}

MatrixXfRow motion_rep_base::translate_2d_to(
    const MatrixXfRow& features, const MatrixXfRow& target_2d_pos, std::int64_t batch_size,
    std::int64_t time_steps
) const {
  // 获取当前第 0 帧的根位置 (xz)
  MatrixXfRow root_pos = get_root_pos(features, batch_size, time_steps);

  MatrixXfRow delta_pos(batch_size, 2);
  for (Eigen::Index b = 0; b < batch_size; ++b) {
    const Eigen::Index idx = b * time_steps;
    delta_pos(b, 0)        = target_2d_pos(b, 0) - root_pos(idx, 0);  // dx
    delta_pos(b, 1)        = target_2d_pos(b, 1) - root_pos(idx, 2);  // dz
  }

  return translate_2d(features, delta_pos, batch_size, time_steps);
}

MatrixXfRow motion_rep_base::translate_2d_to_zero(
    const MatrixXfRow& features, std::int64_t batch_size, std::int64_t time_steps
) const {
  MatrixXfRow zero_pos = MatrixXfRow::Zero(batch_size, 2);
  return translate_2d_to(features, zero_pos, batch_size, time_steps);
}

MatrixXfRow motion_rep_base::canonicalize(
    const MatrixXfRow& features, bool normalized, std::int64_t batch_size, std::int64_t time_steps
) const {
  MatrixXfRow feats = features;
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
