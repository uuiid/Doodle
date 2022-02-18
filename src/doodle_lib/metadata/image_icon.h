//
// Created by TD on 2022/1/21.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <utility>
#include <opencv2/core.hpp>

namespace doodle {
class DOODLELIB_API image_icon {
 public:
  /**
   * @brief 这个路径是相对于根目录的
   */

  image_icon() = default;
  explicit image_icon(FSys::path in_path)
      : path(std::move(in_path)),
        image(){};

  FSys::path path;
  std::shared_ptr<void> image;

  cv::Size2d size2d_;
  friend void to_json(nlohmann::json &j, const image_icon &p);
  friend void from_json(const nlohmann::json &j, image_icon &p);
};
void to_json(nlohmann::json &j, const image_icon &p);
void from_json(const nlohmann::json &j, image_icon &p);
}  // namespace doodle
