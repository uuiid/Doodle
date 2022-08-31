//
// Created by TD on 2022/4/29.
//

#include "rpc_server.h"

namespace doodle::json_rpc {
void rpc_server::register_fun(const std::string& in_name, const rpc_server::call_fun& in_call) {
  fun_list_.emplace(in_name, in_call);
}
rpc_server::call_ rpc_server::operator()(const std::string& in_name) const {
  if (fun_list_.find(in_name) != fun_list_.end()) {
    return fun_list_.at(in_name);
  } else {
    throw method_not_found_exception{};
  }
}
rpc_server::rpc_server()
    : fun_list_() {
}
rpc_server::~rpc_server() = default;

}  // namespace doodle::json_rpc
