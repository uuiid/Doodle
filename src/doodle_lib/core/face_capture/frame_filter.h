//
// Created by TD on 2026-11-6.
//

#pragma once

#include <doodle_lib/core/face_capture/face_capture_types.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <string>

namespace doodle::face_capture {

struct frame_filter_option {
  std::int32_t width_{0};
  std::int32_t height_{0};
  std::string graph_{"fps=25"};
};

using frame_filter_sink_t = std::function<void(const cv::Mat&, std::uint64_t)>;

class frame_filter {
 public:
  static std::shared_ptr<frame_filter> make(const frame_filter_option& in_option, frame_filter_sink_t in_sink);
  frame_filter(const frame_filter_option& in_option, frame_filter_sink_t in_sink);

  frame_filter(frame_filter&&) noexcept;
  frame_filter& operator=(frame_filter&&) noexcept;

  ~frame_filter();

  bool valid() const;

  void push(const cv::Mat& in_bgr, std::int64_t in_pts_us);

  void flush();

  std::uint64_t output_frames() const;

  std::uint64_t inserted_frames() const;

  void add_inserted();

 private:
  class impl;
  std::unique_ptr<impl> impl_ptr_{};
};

}  // namespace doodle::face_capture
