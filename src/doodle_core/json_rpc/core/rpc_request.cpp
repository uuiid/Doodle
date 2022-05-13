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
  if (!nlohmann_json_t.is_notice)
    nlohmann_json_j["id"] = rpc_request::identifier::get().id();
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
}  // namespace doodle::json_rpc
