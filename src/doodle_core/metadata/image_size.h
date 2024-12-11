//
// Created by TD on 24-12-11.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle {
class DOODLE_CORE_API image_size {
 public:

  std::int32_t width ;
  std::int32_t height ;
  friend  void to_json(nlohmann::json& j, const image_size& p) { j["width"] = p.width; j["height"] = p.height; }
  friend void from_json(const nlohmann::json& j, image_size& p) { j["width"].get_to(p.width); j["height"].get_to(p.height); }

};
}