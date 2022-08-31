//
// Created by TD on 2022/5/9.
//

#include "json_rpc_client.h"

#include <json_rpc/json_rpc_static_value.h>
#include <json_rpc/args/rpc_json_progress.h>
#include <metadata/move_create.h>
#include <metadata/project.h>

namespace doodle {

json_rpc_client::json_rpc_client(boost::asio::io_context& in_context, const std::string& in_host, uint16_t in_post)
    : rpc_client(in_context, in_host, in_post) {}
void json_rpc_client::image_to_move(const std::vector<movie::image_attr>& in_list) {
  call_fun<void, false>(json_rpc::rpc_fun_name::image_to_move, in_list);
}
json_rpc::args::rpc_json_progress json_rpc_client::get_progress() {
  return call_fun<json_rpc::args::rpc_json_progress>(json_rpc::rpc_fun_name::get_progress);
}
void json_rpc_client::open_project(const FSys::path& in_path) {
  call_fun<void, false>(json_rpc::rpc_fun_name::open_project, in_path);
}
project_config::base_config json_rpc_client::get_project_config() {
  return call_fun<project_config::base_config, false>(json_rpc::rpc_fun_name::get_project_config);
}

}  // namespace doodle
