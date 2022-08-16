//
// Created by TD on 2022/1/21.
//

#include "image_icon.h"
#include <doodle_core/metadata/project.h>
#include <boost/contract.hpp>
#include <doodle_core/logger/logger.h>

namespace doodle {
FSys::path image_icon::image_root(const entt::handle& in_handle) const {
  FSys::path result;
  boost::contract::check l_c =
      boost::contract::public_function(this)
          .precondition([&]() {
            DOODLE_CHICK(in_handle,doodle_error{"无效的句柄"});
          })
          .postcondition([&]() {
            DOODLE_CHICK(!result.empty(),doodle_error{"无效的根目录"});
          });

  return result = in_handle
                      .registry()
                      ->ctx()
                      .at<project>()
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
