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
json_rpc::args::rpc_json_progress json_rpc_client::image_to_move(const std::vector<movie::image_attr>& in_list) {
  return call_fun<json_rpc::args::rpc_json_progress>(json_rpc::rpc_fun_name::image_to_move, in_list);
}

}  // namespace doodle
