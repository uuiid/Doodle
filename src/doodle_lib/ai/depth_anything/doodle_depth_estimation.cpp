//
// Created by TD on 25-7-12.
//
// PIMPL 实现 — 将 depth_anything.hpp 的编译隔离在此 .cpp 中
//

#include "doodle_depth_estimation.h"

// 仅在 .cpp 中包含 Depths-CPP 头文件，避免 ONNX Runtime 头文件污染公共 API
#include <depth_anything.hpp>

namespace doodle::ai {

class doodle_depth_estimation::impl {
 public:
  depth::Config config_;
  std::unique_ptr<DepthAnything> engine_;
};
doodle_depth_estimation::doodle_depth_estimation() = default;
doodle_depth_estimation::doodle_depth_estimation(const std::filesystem::path& in_model_path, bool in_use_cuda)
    : impl_(std::make_unique<impl>()) {
  impl_->config_.modelPath   = in_model_path.generic_string();
  impl_->config_.process_res = 504;
  impl_->config_.provider    = in_use_cuda ? depth::Provider::Auto : depth::Provider::CPU;
  impl_->engine_             = std::make_unique<DepthAnything>(impl_->config_);
}
// move constructor and move assignment operator
doodle_depth_estimation::doodle_depth_estimation(doodle_depth_estimation&&) noexcept            = default;
doodle_depth_estimation& doodle_depth_estimation::operator=(doodle_depth_estimation&&) noexcept = default;
doodle_depth_estimation::~doodle_depth_estimation()                                             = default;

cv::Mat doodle_depth_estimation::predict(const cv::Mat& in_image) { return impl_->engine_->predict(in_image); }

std::vector<cv::Mat> doodle_depth_estimation::predict_batch(const std::vector<cv::Mat>& in_images) {
  return impl_->engine_->predictBatch(in_images);
}

bool doodle_depth_estimation::is_metric() const { return impl_->engine_->isMetric(); }
doodle_depth_estimation::operator bool() const { return static_cast<bool>(impl_ && impl_->engine_); }

}  // namespace doodle::ai