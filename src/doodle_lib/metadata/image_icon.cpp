//
// Created by TD on 2022/1/21.
//

#include "image_icon.h"

namespace doodle {
void to_json(nlohmann::json& j, const doodle::image_icon& p) {
  j["path"] = p.path;
}
void from_json(const nlohmann::json& j, doodle::image_icon& p) {
  j["path"].get_to(p.path);
}

}  // namespace doodle
