//
// Created by TD on 2022/5/9.
//

#include "json_rpc_server_i.h"

#include <json_rpc/json_rpc_static_value.h>
#include <metadata/move_create.h>
namespace doodle {
void json_rpc_server_i::init_register() {
  register_fun(json_rpc::rpc_fun_name::image_to_move, [this](const json_sig& in_skin, const std::optional<nlohmann::json>& in_json) {
    image_to_move_sig l_sig{};
    l_sig.connect([&](const json_rpc::args::rpc_json_progress& in_progress) {
      nlohmann::json l_json{};
      l_json = in_progress;
      in_skin(l_json);
    });

    this->create_movie(l_sig, in_json->get<std::vector<movie::image_attr>>());
  });
}
}  // namespace doodle
