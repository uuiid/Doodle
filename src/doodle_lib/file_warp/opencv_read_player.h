//
// Created by TD on 2021/11/04.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <opencv2/opencv.hpp>
namespace cv {
class VideoCapture;
}
namespace doodle {
using video_capture_ptr = std::shared_ptr<cv::VideoCapture>;
}

namespace doodle {

class DOODLELIB_API opencv_read_player {
  cv::VideoCapture p_video;
 public:
  opencv_read_player();

  bool is_open() const;

  bool open_file(const FSys::path& in_path);
  /**
   * @brief 读取视频中的帧并送入gpu ， 返回值为gup 后端纹理，可以直接送如imgui
   *
   * @param in_frame
   * @return std::tuple<void*, std::pair<std::int32_t,std::int32_t> >
   */
  std::tuple<void*, std::pair<std::int32_t, std::int32_t> > read(std::int32_t in_frame);
};
}  // namespace doodle