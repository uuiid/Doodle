//
// Created by TD on 2022/5/9.
//

#include "json_rpc_server_i.h"

#include <json_rpc/json_rpc_static_value.h>
#include <metadata/move_create.h>
namespace doodle {
void json_rpc_server_i::init_register() {
  register_fun(
      json_rpc::rpc_fun_name::image_to_move,
      [this](const std::optional<nlohmann::json>& in_json) {
        return this->create_movie(in_json->get<create_move_arg>());
      }
  );
}
}  // namespace doodle
