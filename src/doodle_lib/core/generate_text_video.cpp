#include "generate_text_video.hpp"

#include "doodle_core/core/file_sys.h"

#include <filesystem>
#include <opencv2/freetype.hpp>
#include <opencv2/opencv.hpp>

namespace doodle {

void generate_text_video::run() const {
  DOODLE_CHICK(!out_path_.empty(), "ffmpeg_video: output path is empty");
  auto out_path = out_path_;
  out_path      = out_path.replace_extension(".mp4");
  auto l_backup = FSys::add_time_stamp(out_path);

  if (auto l_p = out_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);

  {
    cv::VideoWriter l_writer{
        l_backup.generic_string(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'), 25.0,
        cv::Size(size_.width, size_.height)
    };
    // l_writer.open()
    DOODLE_CHICK(l_writer.isOpened(), "无法创建视频文件: {} ", l_backup);
    auto k_image            = cv::Mat{size_.height, size_.width, CV_8UC3, cv::Scalar(0, 0, 0)};
    const auto total_frames = duration_.count() * 25;
    auto const l_ft2{cv::freetype::createFreeType2()};
    for (const auto& font_attr : font_attrs_) {
      l_ft2->loadFontData(font_attr.font_path_.generic_string(), 0);
      const auto l_font_size = l_ft2->getTextSize(font_attr.text_, font_attr.font_height_, -1, nullptr);
      cv::Point l_textOrg((size_.width - l_font_size.width) / 2, (size_.height + l_font_size.height) / 2);
      if (font_attr.font_point_.y != 0) l_textOrg.y = font_attr.font_point_.y;
      if (font_attr.font_point_.x != 0) l_textOrg.x = font_attr.font_point_.x;

      l_ft2->putText(
          k_image, font_attr.text_, l_textOrg, font_attr.font_height_, font_attr.font_color_, -1, cv::LINE_AA, false
      );
    }
    for (std::int32_t frame_idx = 0; frame_idx < total_frames; ++frame_idx) l_writer << k_image;
  }
  FSys::rename(l_backup, out_path_);
}

}  // namespace doodle