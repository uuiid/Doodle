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
void json_rpc_client::image_to_move(image_to_move_arg::push_type& in_skin,
                                    const std::vector<movie::image_attr>& in_list) {
  call_fun<json_rpc::args::rpc_json_progress>(json_rpc::rpc_fun_name::image_to_move,
                                              in_skin,
                                              in_list);
}
project json_rpc_client::open_project(const FSys::path& in_path) {
  return call_fun<project>(json_rpc::rpc_fun_name::open_project,
                           in_path);
}

}  // namespace doodle
