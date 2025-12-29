//
// Created by TD on 24-12-11.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle {
class DOODLE_CORE_API image_size {
 public:
  std::int32_t width{0};
  std::int32_t height{0};

  // std::pair<std::int32_t, std::int32_t> 转换函数
  template <typename T>
    requires std::is_integral_v<T>
  image_size& operator=(const std::pair<T, T>& in_pair) {
    width  = in_pair.first;
    height = in_pair.second;
    return *this;
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