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
};

struct ID3D11ShaderResourceView;

namespace doodle {

class DOODLELIB_API opencv_read_player {
  cv::VideoCapture p_video;
  template <class T>
  struct win_ptr_delete {
    void operator()(T* ptr) const {
      ptr->Release();
    }
  };

  struct frame {
    std::unique_ptr<ID3D11ShaderResourceView,
                    win_ptr_delete<ID3D11ShaderResourceView>>
        p_d3d_view;
    std::uint32_t p_frame;
    std::uint32_t p_width;
    std::uint32_t p_height;
    DOODLE_MOVE(frame);
  };
  std::map<std::uint32_t, frame> p_image;

  bool load_frame(std::int32_t in_frame);
  bool clear_cache();

 public:
  opencv_read_player();
  DOODLE_MOVE(opencv_read_player)

  bool is_open() const;

  bool open_file(const FSys::path& in_path);
  /**
   * @brief 读取视频中的帧并送入gpu ， 返回值为gup 后端纹理，可以直接送如imgui
   *
   * @param in_frame
   * @return std::tuple<void*, std::pair<std::int32_t,std::int32_t> >
   *
   * imhui 需要的指针
   * 图像大小
   */
  std::tuple<void*, std::pair<std::int32_t, std::int32_t>> read(std::int32_t in_frame);
};
}  // namespace doodle