//
// Created by TD on 24-12-11.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <boost/numeric/conversion/cast.hpp>
namespace doodle {
class DOODLE_CORE_API image_size {
 public:
  std::int32_t width{0};
  std::int32_t height{0};

  // 默认构造函数
  image_size() = default;

  explicit image_size(const std::int32_t in_width, const std::int32_t in_height) : width(in_width), height(in_height) {}
  template <typename T>
    requires std::is_integral_v<T>
  explicit image_size(const std::pair<T, T>& in_pair)
      : width(boost::numeric_cast<std::int32_t>(in_pair.first)),
        height(boost::numeric_cast<std::int32_t>(in_pair.second)) {}

  // std::pair<std::int32_t, std::int32_t> 转换函数
  template <typename T>
    requires std::is_integral_v<T>
  image_size& operator=(const std::pair<T, T>& in_pair) {
    width  = in_pair.first;
    height = in_pair.second;
    return *this;
  }
  // 比较函数
  bool operator==(const image_size& other) const { return width == other.width && height == other.height; }
  bool operator!=(const image_size& other) const { return !(*this == other); }
  template <typename T>
    requires std::is_integral_v<T>
  bool operator==(const std::pair<T, T>& other) const {
    return width == other.first && height == other.second;
  }
  template <typename T>
    requires std::is_integral_v<T>
  bool operator!=(const std::pair<T, T>& other) const {
    return !(*this == other);
  }

  friend void to_json(nlohmann::json& j, const image_size& p) {
    j["width"]  = p.width;
    j["height"] = p.height;
  }
  friend void from_json(const nlohmann::json& j, image_size& p) {
    j["width"].get_to(p.width);
    j["height"].get_to(p.height);
  }
};
}  // namespace doodle