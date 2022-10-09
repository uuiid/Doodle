//
// Created by TD on 2022/5/9.
//

#include "json_rpc_server_i.h"

#include <json_rpc/json_rpc_static_value.h>
#include <metadata/move_create.h>
namespace doodle {
void json_rpc_server_i::init_register() {
  register_fun_t(
      json_rpc::rpc_fun_name::image_to_move,
      [this](const std::optional<nlohmann::json>& in_json) -> entt::entity {
        return this->create_movie(in_json->get<create_move_arg>());
      }
  );
  register_fun_t(
      json_rpc::rpc_fun_name::get_progress,
      [this](const std::optional<nlohmann::json>& in_json) {
        return this->get_progress(in_json->get<entt::entity>());
      }
  );
  register_fun_t(
      json_rpc::rpc_fun_name::stop_app,
      [this]() {
        return this->stop_app();
      }
  );
}
}  // namespace doodle
