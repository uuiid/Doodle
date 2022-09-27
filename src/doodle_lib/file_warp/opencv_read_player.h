//
// Created by TD on 2021/11/04.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {

namespace opencv {

class frame {
 public:
  std::uint32_t width;
  std::uint32_t height;
  std::uint32_t frame_num;
  void* data;

  template <class T>
  frame& multiply(const T& t) {
    width  = width * t;
    height = height * t;
    return *this;
  }
};
};  // namespace opencv

class DOODLELIB_API opencv_read_player {
  bool load_frame(std::int32_t in_frame);
  bool clear_cache();

 public:
  opencv_read_player();
  virtual ~opencv_read_player();
  DOODLE_IMP_MOVE(opencv_read_player)

  bool is_open() const;

  bool open_file(const FSys::path& in_path);
  /**
   * @brief 读取视频中的帧并送入gpu ， 返回值为gup 后端纹理，可以直接送如imgui
   *
   * @param in_frame
   * @return std::tuple<void*, std::pair<std::int32_t,std::int32_t> >
   *
   * imhui 需要的指针
   * 图像大小 width height
   */
  opencv::frame read(std::int32_t in_frame);

 private:
  class impl;
  std::unique_ptr<impl> p_data;
};
}  // namespace doodle
