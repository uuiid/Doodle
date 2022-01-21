//
// Created by TD on 2022/1/21.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API image_icon {
 public:
  /**
   * @brief 这个路径是相对于根目录的
   */
  FSys::path path;
  std::shared_ptr<void> image;
};

}  // namespace doodle
