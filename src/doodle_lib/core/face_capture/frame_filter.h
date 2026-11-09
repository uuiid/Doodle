//
// Created by TD on 2026-11-6.
//

#pragma once

#include <doodle_lib/core/face_capture/face_capture_types.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <cstdint>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <string>
#include <vector>

namespace doodle::face_capture {

struct frame_filter_option {
  std::int32_t width_{0};
  std::int32_t height_{0};
  std::string graph_{"fps=25"};
};

struct filtered_frame {
  cv::Mat mat_{};
  std::uint64_t seq_{0};
};

class frame_filter {
 public:
  explicit frame_filter(const frame_filter_option& in_option);

  frame_filter(frame_filter&&) noexcept;
  frame_filter& operator=(frame_filter&&) noexcept;

  ~frame_filter();

  void push(const cv::Mat& in_bgr, std::int64_t in_pts_us);

  void flush();

  std::vector<filtered_frame> take_frames();

  std::uint64_t output_frames() const;

 private:
  class impl;
  std::unique_ptr<impl> impl_ptr_{};
};

}  // namespace doodle::face_capture
