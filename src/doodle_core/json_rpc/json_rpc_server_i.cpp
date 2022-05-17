//
// Created by TD on 2022/5/9.
//

#include "json_rpc_server_i.h"

#include <json_rpc/json_rpc_static_value.h>
#include <metadata/move_create.h>
namespace doodle {
void json_rpc_server_i::init_register() {
  register_fun(json_rpc::rpc_fun_name::image_to_move,
               [this](json_coroutine::push_type& in_skin,
                      const std::optional<nlohmann::json>& in_json) {
                 image_to_move_arg::pull_type l_fun{
                     [this, in_json](image_to_move_arg::push_type& in_skin_arg) {
                       this->create_movie(in_skin_arg,
                                          in_json->get<std::vector<movie::image_attr>>());
                     }};

                 for (auto&& obj : l_fun) {
                   nlohmann::json l_json{};
                   l_json = obj;
                   in_skin(l_json);
                 }
               });
  register_fun(json_rpc::rpc_fun_name::open_project,
               [this](const std::optional<nlohmann::json>& in_json) {
                 nlohmann::json l_json{};
                 l_json = open_project(in_json->get<FSys::path>());
                 return l_json;
               });
}
}  // namespace doodle
