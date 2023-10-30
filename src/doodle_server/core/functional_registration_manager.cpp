//
// Created by td_main on 2023/9/19.
//

#include "functional_registration_manager.h"
namespace doodle::web_socket {

void functional_registration_manager::register_function(
    std::string in_name, std::function<nlohmann::json(const entt::handle &, const nlohmann::json &)> in_function
) {
  map_.emplace(std::move(in_name), std::move(in_function));
}

std::function<nlohmann::json(const entt::handle &, const nlohmann::json &)>
functional_registration_manager::get_function(std::string in_name) {
  if (map_.find(in_name) == map_.end()) return {};
  return map_.at(std::move(in_name));
}

}  // namespace doodle::render_farm