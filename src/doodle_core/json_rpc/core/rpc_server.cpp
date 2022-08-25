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

void rpc_server::register_fun(const std::string& in_name, const rpc_server::call_fun_coroutines& in_call) {
  fun_list_.emplace(in_name, in_call);
}
rpc_server_ref::rpc_server_ref(std::weak_ptr<rpc_server> in_server, const std::function<void()>& in_close_fun)
    : rpc_server(),
      server(std::move(in_server)) {
  fun_list_.emplace(rpc_close_name, [=](const std::optional<nlohmann::json>&) -> rpc_reply {
    in_close_fun();
    return {};
  });
}
rpc_server_ref::call_ rpc_server_ref::operator()(const std::string& in_name) const {
  if (fun_list_.find(in_name) != fun_list_.end()) {
    return fun_list_.at(in_name);
  } else {
    return (*server.lock())(in_name);
  }
}
void rpc_server_ref::close_current() {
  auto l_f = (*this)(rpc_close_name);
  std::get<call_fun>(l_f)({});
}
}  // namespace doodle::json_rpc
