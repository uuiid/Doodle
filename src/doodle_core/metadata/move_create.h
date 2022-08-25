//
// Created by TD on 2022/5/16.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle::movie {
class DOODLE_CORE_EXPORT image_watermark {
 private:
  DOODLE_JSON(image_watermark)
 public:
  using rgba_t      = std::array<std::double_t, 4>;
  image_watermark() = default;
  image_watermark(std::string in_p_text, double_t in_p_width_proportion, double_t in_p_height_proportion, rgba_t in_rgba)
      : text_(std::move(in_p_text)),
        width_proportion_(in_p_width_proportion),
        height_proportion_(in_p_height_proportion),
        rgba_(in_rgba) {}
  std::string text_;
  std::double_t width_proportion_;
  std::double_t height_proportion_;
  rgba_t rgba_;
};

class DOODLE_CORE_EXPORT image_attr {
 private:
  DOODLE_JSON(image_attr)
 public:
  image_attr() = default;
  explicit image_attr(FSys::path in_path)
      : path_(std::move(in_path)){};
  FSys::path path_;
  std::vector<image_watermark> watermarks;
  std::int32_t num;

  static void extract_num(std::vector<image_attr>& in_image_list);
  bool operator<(const image_attr& in_rhs) const;
  bool operator>(const image_attr& in_rhs) const;
  bool operator<=(const image_attr& in_rhs) const;
  bool operator>=(const image_attr& in_rhs) const;
};

}  // namespace doodle::movie
