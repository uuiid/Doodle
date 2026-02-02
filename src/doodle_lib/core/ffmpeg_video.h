//
// Created by TD on 25-2-28.
//

#pragma once

#include <doodle_core/core/file_sys.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/image_size.h>

#include <filesystem>
#include <opencv2/core/types.hpp>
#include <string>
#include <utility>

namespace doodle {
class ffmpeg_video {
  class impl;

  // 主要处理的视频
  FSys::path video_path_;
  // 输出文件
  FSys::path out_path_;
  // 字幕文件
  FSys::path subtitle_path_;
  // 混音文件
  FSys::path audio_path_;
  // 片头
  FSys::path intro_path_;
  // 集数文件
  FSys::path episodes_name_path_;
  // 片尾
  FSys::path outro_path_;
  // 水印
  std::string watermark_text_;
  // 时间码
  bool time_code_{false};

 public:
  explicit ffmpeg_video(const FSys::path& in_video_path, const FSys::path& in_out_path);
  ffmpeg_video();

  ~ffmpeg_video();
  void set_subtitle(const FSys::path& in_subtitle_path) { subtitle_path_ = in_subtitle_path; }
  void set_audio(const FSys::path& in_audio_path) { audio_path_ = in_audio_path; }
  void set_intro(const FSys::path& in_intro_path) { intro_path_ = in_intro_path; }
  void set_outro(const FSys::path& in_outro_path) { outro_path_ = in_outro_path; }
  void set_watermark(const std::string& in_watermark_text) { watermark_text_ = in_watermark_text; }
  void set_time_code(bool in_time_code) { time_code_ = in_time_code; }
  void set_episodes_name(const FSys::path& in_episodes_name_path) { episodes_name_path_ = in_episodes_name_path; }

  void set_input_video(const FSys::path& in_video_path) { video_path_ = in_video_path; }
  void set_output_video(const FSys::path& in_out_path) { out_path_ = in_out_path; }
  // 开始处理
  void process();

  /// 检查视频文件是否有效
  static void check_video_valid(
      const FSys::path& in_video_path, const std::string& in_video_name, bool has_video_stream = true
  );
};

// 重新调整大小
class ffmpeg_video_resize {
  cv::Size target_size_;
  // 低分辨率
  cv::Size low_size_;
  FSys::path video_path_;
  FSys::path out_high_path_;
  FSys::path out_low_path_;
  class impl;

 public:
  explicit ffmpeg_video_resize(
      FSys::path in_video_path, FSys::path in_out_high_path, FSys::path in_out_low_path, const cv::Size& in_size
  )
      : video_path_(std::move(in_video_path)),
        out_high_path_(std::move(in_out_high_path)),
        out_low_path_(std::move(in_out_low_path)),
        target_size_(std::move(in_size)) {
    // 低分辨率定义为宽度小于等于 1280
    low_size_ = cv::Size{1280, static_cast<int>(1280.0 / target_size_.width * target_size_.height)};
  }
  ~ffmpeg_video_resize() = default;
  // 调整视频大小
  void process();
};

}  // namespace doodle