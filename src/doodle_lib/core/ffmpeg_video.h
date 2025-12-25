#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <string>

namespace doodle {
class ffmpeg_video {
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
  // 片尾
  FSys::path outro_path_;
  // 水印
  std::string watermark_text_;
  // 时间码
  bool time_code_{false};

 public:
  explicit ffmpeg_video(const FSys::path& in_video_path, const FSys::path& in_out_path)
      : video_path_(in_video_path), out_path_(in_out_path) {}

  void set_subtitle(const FSys::path& in_subtitle_path) { subtitle_path_ = in_subtitle_path; }
  void set_audio(const FSys::path& in_audio_path) { audio_path_ = in_audio_path; }
  void set_intro(const FSys::path& in_intro_path) { intro_path_ = in_intro_path; }
  void set_outro(const FSys::path& in_outro_path) { outro_path_ = in_outro_path; }
  void set_watermark(const std::string& in_watermark_text) { watermark_text_ = in_watermark_text; }
  void set_time_code(bool in_time_code) { time_code_ = in_time_code; }
  // 开始处理
  void process();
};
}  // namespace doodle