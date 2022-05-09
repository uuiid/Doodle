//
// Created by TD on 2022/4/29.
//

#include "parser_rpc.h"
#include <json_rpc/core/rpc_server.h>

namespace doodle::json_rpc {

std::string parser_rpc::operator()(const rpc_server_ref& in_server) {
  auto rpc_requrst_json = nlohmann::json::parse(json_data_);
  nlohmann::json result{};
  if (rpc_requrst_json.is_array()) {
    for (auto&& i : rpc_requrst_json) {
      auto rpc_requrst_ = rpc_requrst_json.get<rpc_request>();
      auto l_r          = call_one(rpc_requrst_, in_server);
      if (l_r)
        result.emplace_back(*l_r);
    }
  } else {
    auto rpc_requrst_ = rpc_requrst_json.get<rpc_request>();
    auto l_r          = call_one(rpc_requrst_, in_server);
    if (l_r)
      result = *l_r;
  }

  return result.dump();
}

std::optional<rpc_reply>
parser_rpc::call_one(const rpc_request& in_request,
                     const rpc_server_ref& in_server) {
  auto l_r = in_server(in_request.method_, in_request.params_);
  if (in_request.is_notice) {
    return {};
  } else {
    l_r.jsonrpc_ = in_request.jsonrpc_;
    if (l_r.result.index() == rpc_reply::err_index) {
      l_r.id_ = std::monostate{};
    } else {
      l_r.id_ = in_request.id_;
    }
  }
  return l_r;
}
rpc_request::identifier& rpc_request::identifier::get() {
  static identifier identifier1;
  return identifier1;
}
std::uint64_t rpc_request::identifier::id() {
  return ++id_;
}
}  // namespace doodle::json_rpc
