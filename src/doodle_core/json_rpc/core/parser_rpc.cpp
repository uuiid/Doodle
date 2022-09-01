//
// Created by TD on 2022/4/29.
//

#include "parser_rpc.h"
#include <json_rpc/core/rpc_server.h>
#include <doodle_core/json_rpc/core/rpc_request.h>

namespace doodle::json_rpc {

std::string parser_rpc::operator()(const rpc_server& in_server) {
  auto rpc_requrst_json = nlohmann::json::parse(json_data_);
  nlohmann::json result{};
  auto rpc_requrst_ = rpc_requrst_json.get<rpc_request>();

  if (rpc_requrst_.method_ == "rpc.close") {
    is_close = true;
    return {};
  }

  auto l_fun = in_server(rpc_requrst_.method_);

  rpc_reply l_rpc_reply{};
  l_rpc_reply.result = l_fun(rpc_requrst_.params_);
  l_rpc_reply.id_    = rpc_requrst_.id_;
  nlohmann::json l_json{};
  l_json = l_rpc_reply;
  return l_json.dump();
}

void parser_rpc::json_data_attr(const std::string& in_string) {
  json_data_ = in_string;
}
parser_rpc::operator bool() const {
  return is_close;
}

rpc_request::identifier& rpc_request::identifier::get() {
  static identifier identifier1;
  return identifier1;
}
std::uint64_t rpc_request::identifier::id() {
  return ++id_;
}
}  // namespace doodle::json_rpc
