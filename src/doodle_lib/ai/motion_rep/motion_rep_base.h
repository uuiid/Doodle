//
// Created by TD on 25-7-21.
//
#pragma once

#include <doodle_lib/ai/fwd.h>
#include <doodle_lib/ai/motion_rep/feature_utils.h>
#include <doodle_lib/ai/skeleton/skeleton_base.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <Eigen/Dense>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


namespace doodle::ai {

// ======================================================================
// 统计信息（对应 Python Stats）
// ======================================================================

/// @brief 特征标准化统计工具
/// 加载 mean.npy / std.npy，提供 normalize / unnormalize。
struct motion_stats {
  Eigen::VectorXd mean;  ///< [D] 均值
  Eigen::VectorXd std;   ///< [D] 标准差
  static constexpr float eps = 1e-5f;

  /// @brief 从文件夹加载 mean.npy 和 std.npy
  void load(const FSys::path& folder);

  /// @brief 从内存注册统计值
  void set_from_vectors(const Eigen::VectorXf& in_mean, const Eigen::VectorXf& in_std) {
    mean = in_mean;
    std  = in_std;
  }

  /// @brief 标准化: (data - mean) / sqrt(std^2 + eps)
  /// @param data [N, D]
  /// @return [N, D]
  MatrixXfRow normalize(const MatrixXfRow& data) const;

  /// @brief 反标准化: data * sqrt(std^2 + eps) + mean
  /// @param data [N, D]
  /// @return [N, D]
  MatrixXfRow unnormalize(const MatrixXfRow& data) const;

  [[nodiscard]] bool is_valid() const { return mean.size() > 0 && std.size() > 0; }
  [[nodiscard]] std::int64_t dim() const { return mean.size(); }
};

// ======================================================================
// 运动表示基类（对应 Python MotionRepBase）
// ======================================================================

/// @brief 运动表示基类
///
/// 定义特征布局（feature blocks 和 slicing）、标准化和通用几何变换。
/// 子类（如 kimodo_motion_rep）定义具体的 size_dict 和变换方法。
class motion_rep_base {
 protected:
  // ---- 特征布局（由子类构造函数初始化） ----
  std::vector<std::string> feature_names_;   ///< 特征块名称列表
  std::vector<std::int64_t> feature_sizes_;  ///< 各特征块元素数（如 {3, 2, J*3, J*6, J*3, 4}）
  std::int64_t motion_rep_dim_{};            ///< 特征总维度
  std::int64_t nbjoints_{};                  ///< 关节数量

  // ---- 切片信息 ----
  std::unordered_map<std::string, std::int64_t> feature_start_;  ///< 各特征块起始索引
  std::unordered_map<std::string, std::int64_t> feature_end_;    ///< 各特征块结束索引（不含）

  // ---- 根/身体切片（由子类在构造函数中设置） ----
  std::string last_root_feature_;   ///< 根特征块最后一个的名称
  std::int64_t global_root_dim_{};  ///< 全局根维度（root_slice 大小）
  std::int64_t body_dim_{};         ///< 身体维度（body_slice 大小）
  std::int64_t local_root_dim_{};   ///< 局部根维度

  float fps_{30.0f};  ///< 帧率

  // ---- 标准化统计（若提供 stats_path） ----
  motion_stats global_root_stats_;  ///< 全局根统计
  motion_stats local_root_stats_;   ///< 局部根统计
  motion_stats body_stats_;         ///< 身体统计
  motion_stats combined_stats_;     ///< 合并统计（global_root + body）

  // ---- 骨骼（共享指针） ----
  std::shared_ptr<skeleton_base> skeleton_;

  // ======================================================================
  // 子类辅助方法
  // ======================================================================

  /// @brief 从 feature_names_ / feature_sizes_ 构建切片映射
  void build_slice_dict();

  /// @brief 从 stats_path 加载统计（期望子目录 global_root/, local_root/, body/）
  void load_stats(const FSys::path& stats_path);

 public:
  motion_rep_base()          = default;
  virtual ~motion_rep_base() = default;

  // ======================================================================
  // Getters
  // ======================================================================
  [[nodiscard]] std::int64_t motion_rep_dim() const { return motion_rep_dim_; }
  [[nodiscard]] std::int64_t nbjoints() const { return nbjoints_; }
  [[nodiscard]] std::int64_t global_root_dim() const { return global_root_dim_; }
  [[nodiscard]] std::int64_t body_dim() const { return body_dim_; }
  [[nodiscard]] std::int64_t local_root_dim() const { return local_root_dim_; }
  [[nodiscard]] float fps() const { return fps_; }
  [[nodiscard]] const std::shared_ptr<skeleton_base>& skel() const { return skeleton_; }
  [[nodiscard]] skeleton_base& skel_ref() const { return *skeleton_; }

  [[nodiscard]] std::int64_t feature_start(const std::string& name) const { return feature_start_.at(name); }
  [[nodiscard]] std::int64_t feature_end(const std::string& name) const { return feature_end_.at(name); }
  [[nodiscard]] std::int64_t feature_size(const std::string& name) const;

  // ---- 根切片 ----
  [[nodiscard]] std::int64_t root_slice_start() const { return 0; }
  [[nodiscard]] std::int64_t root_slice_end() const { return global_root_dim_; }
  [[nodiscard]] std::int64_t body_slice_start() const { return global_root_dim_; }
  [[nodiscard]] std::int64_t body_slice_end() const { return motion_rep_dim_; }

  // ======================================================================
  // 标准化
  // ======================================================================

  /// @brief 合并标准化（global_root + body，对应 self.stats.normalize）
  MatrixXfRow normalize(const MatrixXfRow& features) const;

  /// @brief 合并反标准化
  MatrixXfRow unnormalize(const MatrixXfRow& features) const;

  // ======================================================================
  // 根位置提取
  // ======================================================================

  /// @brief 从特征中提取根位置
  /// @param features [B*T, motion_rep_dim]
  /// @param batch_size B
  /// @param time_steps T
  /// @return [B*T, 3] 根位置
  MatrixXfRow get_root_pos(const MatrixXfRow& features, std::int64_t batch_size, std::int64_t time_steps) const;

  // ======================================================================
  // 全局根 → 局部根转换（对应 Python MotionRepBase.global_root_to_local_root）
  // ======================================================================

  /// @brief 将全局根特征转换为局部根运动特征
  /// @param root_features [B*T, global_root_dim] 全局根特征（smooth_root_pos + global_root_heading）
  /// @param normalized root_features 是否已标准化
  /// @param batch_size B
  /// @param time_steps T
  /// @param lengths [B] 各样本有效帧数
  /// @return [B*T, local_root_dim] 局部根特征
  MatrixXfRow global_root_to_local_root(
      const MatrixXfRow& root_features, bool normalized, std::int64_t batch_size, std::int64_t time_steps,
      const Eigen::VectorXi& lengths
  ) const;

  // ======================================================================
  // 朝向角提取
  // ======================================================================

  /// @brief 从特征中提取头部朝向角
  /// @param features [B*T, motion_rep_dim]
  /// @param batch_size B
  /// @param time_steps T
  /// @return [B, T] 朝向角（弧度）
  MatrixXfRow get_root_heading_angle(
      const MatrixXfRow& features, std::int64_t batch_size, std::int64_t time_steps
  ) const;

  // ======================================================================
  // 几何变换（由子类重写）
  // ======================================================================

  /// @brief 旋转特征（子类应实现具体旋转逻辑）
  virtual MatrixXfRow rotate(
      const MatrixXfRow& features, const Eigen::VectorXf& angle, std::int64_t batch_size, std::int64_t time_steps
  ) const = 0;

  /// @brief 平移特征（子类应实现具体平移逻辑）
  virtual MatrixXfRow translate_2d(
      const MatrixXfRow& features, const MatrixXfRow& translation_2d, std::int64_t batch_size, std::int64_t time_steps
  ) const = 0;

  // ======================================================================
  // 复合变换（基于 rotate / translate_2d）
  // ======================================================================

  /// @brief 将序列旋转使第 0 帧朝向 target_angle
  MatrixXfRow rotate_to(
      const MatrixXfRow& features, const Eigen::VectorXf& target_angle, std::int64_t batch_size, std::int64_t time_steps
  ) const;

  /// @brief 将序列旋转使第 0 帧朝向 0
  MatrixXfRow rotate_to_zero(const MatrixXfRow& features, std::int64_t batch_size, std::int64_t time_steps) const;

  /// @brief 平移使第 0 帧根位置到 target_2d_pos（xz 平面）
  MatrixXfRow translate_2d_to(
      const MatrixXfRow& features, const MatrixXfRow& target_2d_pos, std::int64_t batch_size, std::int64_t time_steps
  ) const;

  /// @brief 平移使第 0 帧根位置到原点
  MatrixXfRow translate_2d_to_zero(const MatrixXfRow& features, std::int64_t batch_size, std::int64_t time_steps) const;

  /// @brief 归一化：旋转使第 0 帧朝向 0，平移使第 0 帧根位置到原点
  MatrixXfRow canonicalize(
      const MatrixXfRow& features, bool normalized, std::int64_t batch_size, std::int64_t time_steps
  ) const;
};

}  // namespace doodle::ai
