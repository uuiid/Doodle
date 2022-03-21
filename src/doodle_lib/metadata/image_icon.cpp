//
// Created by TD on 2022/1/21.
//

#include "image_icon.h"
#include <metadata/project.h>
#include <boost/contract.hpp>

namespace doodle {
FSys::path image_icon::image_root(const entt::handle& in_handle) const {
  FSys::path result;
  boost::contract::check l_c =
      boost::contract::public_function(this)
          .precondition([&]() {
            chick_true<doodle_error>(in_handle, DOODLE_LOC, "无效的句柄");
          })
          .postcondition([&]() {
            chick_true<doodle_error>(!result.empty(), DOODLE_LOC, "无效的根目录");
          });

  return result = in_handle
                      .registry()
                      ->ctx<project>()
                      .make_path(
                          std::string{doodle_config::image_folder_name});
}

void to_json(nlohmann::json& j, const doodle::image_icon& p) {
  j["path"] = p.path;
}
void from_json(const nlohmann::json& j, doodle::image_icon& p) {
  j["path"].get_to(p.path);
}

}  // namespace doodle
