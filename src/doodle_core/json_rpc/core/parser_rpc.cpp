//
// Created by TD on 2022/4/29.
//

#include "parser_rpc.h"
#include <json_rpc/core/rpc_server.h>

namespace doodle::json_rpc {
void to_json(nlohmann::json& nlohmann_json_j, const rpc_request& nlohmann_json_t) {
  nlohmann_json_j["jsonrpc"] = jsonrpc_version;
  nlohmann_json_j["method"]  = nlohmann_json_t.method_;
  if (!nlohmann_json_t.is_notice)
    nlohmann_json_j["id"] = identifier::get().id();
  if (nlohmann_json_t.params_)
    nlohmann_json_j["params"] = *nlohmann_json_t.params_;
}
void from_json(const nlohmann::json& nlohmann_json_j, rpc_request& nlohmann_json_t) {
  nlohmann_json_j.at("jsonrpc").get_to(nlohmann_json_t.jsonrpc_);
  nlohmann_json_j.at("method").get_to(nlohmann_json_t.method_);
  if (nlohmann_json_j.contains("id")) {
    auto&& l_j = nlohmann_json_j.at("id");
    if (l_j.is_number())
      nlohmann_json_t.id_ = l_j.get<std::int64_t>();
    else if (l_j.is_string())
      nlohmann_json_t.id_ = l_j.get<std::string>();
    else
      throw internal_error_exception{};
  } else {
    nlohmann_json_t.is_notice = true;
  }

  if (nlohmann_json_j.contains("params"))
    nlohmann_json_t.params_ = nlohmann_json_j.at("params");
}
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
void parser_rpc::json_data_attr(const std::string& in_string) {
  json_data_ = in_string;
}
void parser_rpc::operator()(boost::coroutines2::coroutine<std::string>::push_type& sink,
                            const rpc_server_ref& in_server) {
  auto rpc_requrst_json = nlohmann::json::parse(json_data_);
  nlohmann::json result{};
  if (rpc_requrst_json.is_array()) {
    //    for (auto&& i : rpc_requrst_json) {
    //      auto rpc_requrst_ = rpc_requrst_json.get<rpc_request>();
    //      auto l_fun        = in_server(rpc_requrst_.method_);
    //      rpc_reply l_rpc_reply{};
    //      json_coroutine::pull_type l_pull_fun{[&](json_coroutine::push_type& in_skin_la) {
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
    //        sink(fun_i.dump());
    //      }
    //    }
  } else {
    auto rpc_requrst_ = rpc_requrst_json.get<rpc_request>();
    auto l_fun        = in_server(rpc_requrst_.method_);
    rpc_reply l_rpc_reply{};
    json_coroutine::pull_type l_pull_fun{[&](json_coroutine::push_type& in_skin_la) {
      std::visit(detail::overloaded{[&](const rpc_server_ref::call_fun& in_call_fun) {
                                      in_skin_la(in_call_fun(rpc_requrst_.params_));
                                    },
                                    [&](const rpc_server_ref::call_fun_coroutines& in_call_fun) {
                                      in_call_fun(in_skin_la, rpc_requrst_.params_);
                                    }},
                 l_fun);
    }};
    for (auto&& fun_i : l_pull_fun) {
      l_rpc_reply.result = fun_i;
      l_rpc_reply.id_    = rpc_requrst_.id_;
      sink(fun_i.dump());
    }
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
