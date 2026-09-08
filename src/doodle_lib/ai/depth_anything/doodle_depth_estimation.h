//
// Created by TD on 25-7-12.
//
// 深度估计封装类 — 对 Depths-CPP 的 PIMPL 包装
// 将 depth_anything.hpp（含 ONNX Runtime 头文件）的编译隔离在 .cpp 中
//

#pragma once

#include <doodle_lib/ai/depth_anything/depth_anything_fwd.h>
#include <doodle_lib/configure/doodle_lib_export.h>

#include <filesystem>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <vector>

namespace doodle::ai {

class DOODLELIB_API doodle_depth_estimation {
  class impl;

 public:
  doodle_depth_estimation();
  /// @param in_model_path ONNX 模型文件路径
  /// @param in_use_cuda  是否尝试 GPU 加速（Auto 模式：TensorRT → CUDA → CPU）
  explicit doodle_depth_estimation(const std::filesystem::path& in_model_path, bool in_use_cuda = true);
  ~doodle_depth_estimation();

  // move constructor and move assignment operator
  doodle_depth_estimation(doodle_depth_estimation&&) noexcept            = default;
  doodle_depth_estimation& operator=(doodle_depth_estimation&&) noexcept = default;

  // delete copy constructor and copy assignment operator
  doodle_depth_estimation(const doodle_depth_estimation&) = delete;
  doodle_depth_estimation& operator=(const doodle_depth_estimation&) = delete;

  /// 单帧深度估计，返回 CV_32FC1 深度图（输入分辨率）
  cv::Mat predict(const cv::Mat& in_image);

  /// 批量深度估计
  std::vector<cv::Mat> predict_batch(const std::vector<cv::Mat>& in_images);

  /// 是否为度量深度模型（输出单位为米）
  bool is_metric() const;

 private:
  std::unique_ptr<impl> impl_;
};

}  // namespace doodle::ai