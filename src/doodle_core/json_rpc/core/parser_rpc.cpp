//
// Created by TD on 2022/4/29.
//

#include "parser_rpc.h"
#include <json_rpc/core/rpc_server.h>
#include <doodle_core/json_rpc/core/rpc_request.h>

namespace doodle::json_rpc {

std::string parser_rpc::operator()(const rpc_server_ref& in_server) {
  auto rpc_requrst_json = nlohmann::json::parse(json_data_);
  nlohmann::json result{};
  return result.dump();
}

void parser_rpc::json_data_attr(const std::string& in_string) {
  json_data_ = in_string;
}
void parser_rpc::operator()(const string_sig& sink, const rpc_server_ref& in_server) {
  auto rpc_requrst_json = nlohmann::json::parse(json_data_);
  nlohmann::json result{};

  if (rpc_requrst_json.is_array()) {
    //    for (auto&& rpc_i : rpc_requrst_json) {
    //      auto rpc_requrst_ = rpc_i.get<rpc_request>();
    //      auto l_fun        = in_server(rpc_requrst_.method_);
    //      rpc_reply l_rpc_reply{};
    //      json_coroutine::pull_type l_pull_fun{[=](json_coroutine::push_type& in_skin_la) {
    //        std::visit(detail::overloaded{[&](const rpc_server_ref::call_fun& in_call_fun) {
    //                                        in_skin_la(in_call_fun(rpc_requrst_.params_));
    //                                      },
    //                                      [&](const rpc_server_ref::call_fun_coroutines& in_call_fun) {
    //                                        in_call_fun(in_skin_la, rpc_requrst_.params_);
    //                                      }},
    //                   l_fun);
    //      }};
    //      for (auto&& fun_i : l_pull_fun) {
    //        l_rpc_reply.result = fun_i;
    //        l_rpc_reply.id_    = rpc_requrst_.id_;
    //        nlohmann::json  l_json{};
    //        l_json = l_rpc_reply;
    //        sink(l_json.dump());
    //      }
    //    }
  } else {
    auto rpc_requrst_ = rpc_requrst_json.get<rpc_request>();
    auto l_fun        = in_server(rpc_requrst_.method_);
    rpc_server::json_sig l_json_sig{};
    l_json_sig.connect([&](const nlohmann::json& in_json) {
      rpc_reply l_rpc_reply{};
      l_rpc_reply.result = in_json;
      l_rpc_reply.id_    = rpc_requrst_.id_;
      nlohmann::json l_json{};
      l_json = l_rpc_reply;
      sink(l_json.dump());
    });

    std::visit(detail::overloaded{[&](const rpc_server_ref::call_fun& in_call_fun) {
                                    l_json_sig(in_call_fun(rpc_requrst_.params_));
                                  },
                                  [&](const rpc_server_ref::call_fun_coroutines& in_call_fun) {
                                    in_call_fun(l_json_sig, rpc_requrst_.params_);
                                  }},
               l_fun);
  }
}

rpc_request::identifier& rpc_request::identifier::get() {
  static identifier identifier1;
  return identifier1;
}
std::uint64_t rpc_request::identifier::id() {
  return ++id_;
}
}  // namespace doodle::json_rpc
