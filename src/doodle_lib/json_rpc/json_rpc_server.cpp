//
// Created by TD on 2022/5/17.
//

#include "json_rpc_server.h"
#include <doodle_core/metadata/project.h>
#include <doodle_core/core/app_base.h>
#include <long_task/image_to_move.h>

namespace doodle {

class json_rpc_server::impl {
 public:
  impl() = default;
};

json_rpc_server::json_rpc_server()
    : ptr(std::make_unique<impl>()) {
}

entt::entity json_rpc_server::create_movie(
    const create_move_arg& in_arg
) {
  auto l_h = make_handle();
  l_h.emplace<episodes>().analysis(in_arg.out_path);
  l_h.emplace<shot>().analysis(in_arg.out_path);
  l_h.emplace<FSys::path>(in_arg.out_path);
  g_reg()->ctx().at<image_to_move>()->async_create_move(
      l_h, in_arg.image, []() {}
  );
  return l_h;
}
process_message json_rpc_server::get_progress(entt::entity in_id) {
  auto l_h = make_handle(in_id);
  return l_h && l_h.all_of<process_message>()
             ? l_h.get<process_message>()
             : process_message{};
}
void json_rpc_server::stop_app() {
  app_base::Get().stop_app();
}
void json_rpc_server::init_register() {
  register_fun_t(
      rpc_fun_name::image_to_move,
      [this](const std::optional<nlohmann::json>& in_json) -> entt::entity {
        return this->create_movie(in_json->get<create_move_arg>());
      }
  );
  register_fun_t(
      rpc_fun_name::get_progress,
      [this](const std::optional<nlohmann::json>& in_json) {
        return this->get_progress(in_json->get<entt::entity>());
      }
  );
  register_fun_t(
      rpc_fun_name::stop_app,
      [this]() {
        return this->stop_app();
      }
  );
}
json_rpc_server::~json_rpc_server() = default;
}  // namespace doodle
