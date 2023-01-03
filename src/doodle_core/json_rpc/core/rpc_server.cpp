//
// Created by TD on 2022/4/29.
//

#include "rpc_server.h"

namespace doodle::json_rpc {
void rpc_server::register_fun(const std::string& in_name, const rpc_server::call_fun& in_call) {
  fun_list_.emplace(in_name, in_call);
}
std::string rpc_server::operator()(const std::string& in_data) const {
  auto rpc_requrst_json = nlohmann::json::parse(in_data);
  nlohmann::json result{};
  auto rpc_requrst_ = rpc_requrst_json.get<rpc_request>();

  rpc_server::call_ l_call{};
  if (fun_list_.find(rpc_requrst_.method_) != fun_list_.end()) {
    l_call = fun_list_.at(rpc_requrst_.method_);
  } else {
    throw method_not_found_exception{};
  }

  rpc_reply l_rpc_reply{};
  l_rpc_reply.result = l_call(rpc_requrst_.params_);

  if (rpc_requrst_.id_) l_rpc_reply.id_ = *rpc_requrst_.id_;
  nlohmann::json l_json{};
  l_json = l_rpc_reply;
  return l_json.dump();
}
rpc_server::rpc_server() : fun_list_() {}
rpc_server::~rpc_server() = default;

}  // namespace doodle::json_rpc
