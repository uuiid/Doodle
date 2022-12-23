//
// Created by TD on 2022/5/13.
//
#include "rpc_request.h"

#include <doodle_core/json_rpc/exception/json_rpc_error.h>

#include <nlohmann/json.hpp>

namespace doodle::json_rpc {

void to_json(nlohmann::json& nlohmann_json_j, const rpc_request& nlohmann_json_t) {
  nlohmann_json_j["jsonrpc"] = rpc_request::jsonrpc_version;
  nlohmann_json_j["method"]  = nlohmann_json_t.method_;
  if (nlohmann_json_t.id_) {
    std::visit(
        detail::overloaded{
            [&](const std::int64_t& in_id) { nlohmann_json_j["id"] = in_id; },
            [&](const std::string& in_str_id) { nlohmann_json_j["id"] = in_str_id; }},
        *nlohmann_json_t.id_
    );
  }
  if (nlohmann_json_t.params_) nlohmann_json_j["params"] = *nlohmann_json_t.params_;
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
  }
  if (nlohmann_json_j.contains("params")) nlohmann_json_t.params_ = nlohmann_json_j.at("params");
}

void to_json(nlohmann::json& nlohmann_json_j, const rpc_reply& nlohmann_json_t) {
  nlohmann_json_j["jsonrpc"] = nlohmann_json_t.jsonrpc_;
  std::visit(
      detail::overloaded{
          [&](const nlohmann::json& in_json) { nlohmann_json_j["result"] = in_json; },
          [&](const rpc_error& in_error) { nlohmann_json_j["error"] = in_error; }},
      nlohmann_json_t.result
  );
  std::visit(
      detail::overloaded{
          [&](const std::int64_t& in_id) { nlohmann_json_j["id"] = in_id; },
          [&](const std::string& in_str_id) { nlohmann_json_j["id"] = in_str_id; },
          [&](const std::monostate& in_null_id) {
            nlohmann_json_j["id"] = nlohmann::json{nlohmann::json::value_t::null};
          }},
      nlohmann_json_t.id_
  );
}
void from_json(const nlohmann::json& nlohmann_json_j, rpc_reply& nlohmann_json_t) {
  nlohmann_json_j.at("jsonrpc").get_to(nlohmann_json_t.jsonrpc_);
  if (nlohmann_json_j.contains("result")) {
    auto l_r               = nlohmann_json_j.at("result").get<nlohmann::json>();
    nlohmann_json_t.result = l_r;
  } else if (nlohmann_json_j.contains("error")) {
    nlohmann_json_t.result = nlohmann_json_j.at("error").get<rpc_error>();
  } else {
    throw std::runtime_error{"错误"};
  }
  auto&& l_j = nlohmann_json_j.at("id");
  if (l_j.is_number())
    nlohmann_json_t.id_ = l_j.get<std::uint64_t>();
  else if (l_j.is_string())
    nlohmann_json_t.id_ = l_j.get<std::string>();
  else
    throw internal_error_exception{};
}
}  // namespace doodle::json_rpc
