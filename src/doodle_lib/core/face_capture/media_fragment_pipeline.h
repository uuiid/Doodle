//
// Created by TD on 2026-11-6.
//

#pragma once

#include <doodle_lib/core/face_capture/face_capture_types.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <cstdint>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <vector>

namespace doodle::face_capture {

class raw_writer {
 public:
  virtual ~raw_writer() = default;
  virtual bool push(const cv::Mat& in_bgr, std::int64_t in_wall_ms) = 0;
  virtual std::uint64_t frames() const = 0;
  virtual bool finish() = 0;
};

class video_writer {
 public:
  virtual ~video_writer() = default;
  virtual bool push(const cv::Mat& in_bgr, std::uint64_t in_seq) = 0;
  virtual std::uint64_t frames() const = 0;
  virtual bool finish() = 0;
};

class h264_fragmenter : public video_writer {
 public:
  static std::shared_ptr<h264_fragmenter> make(std::int32_t in_width, std::int32_t in_height);
  explicit h264_fragmenter(std::int32_t in_width, std::int32_t in_height);

  h264_fragmenter(h264_fragmenter&&) noexcept;
  h264_fragmenter& operator=(h264_fragmenter&&) noexcept;

  ~h264_fragmenter() override;

  bool push(const cv::Mat& in_bgr, std::uint64_t in_seq) override;
  std::uint64_t frames() const override;
  bool finish() override;

  bool init();

  const std::vector<std::uint8_t>& init_segment() const;

  std::vector<fragment_t> take_fragments();

 private:
  class impl;
  std::unique_ptr<impl> impl_ptr_{};
};

class mp4_writer : public video_writer {
 public:
  static std::shared_ptr<mp4_writer> make(
      const FSys::path& in_path, std::int32_t in_width, std::int32_t in_height, bool in_low_latency
  );
  mp4_writer(const FSys::path& in_path, std::int32_t in_width, std::int32_t in_height, bool in_low_latency);

  mp4_writer(mp4_writer&&) noexcept;
  mp4_writer& operator=(mp4_writer&&) noexcept;

  ~mp4_writer() override;

  bool init();

  bool push(const cv::Mat& in_bgr, std::uint64_t in_seq) override;
  std::uint64_t frames() const override;
  bool finish() override;

 private:
  class impl;
  std::unique_ptr<impl> impl_ptr_{};
};

std::shared_ptr<raw_writer> make_raw_writer(const FSys::path& in_path, std::int32_t in_width, std::int32_t in_height);

std::shared_ptr<video_writer> make_mp4_writer(
    const FSys::path& in_path, std::int32_t in_width, std::int32_t in_height, bool in_low_latency
);

}  // namespace doodle::face_capture
