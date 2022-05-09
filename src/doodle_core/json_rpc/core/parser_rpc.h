//
// Created by TD on 2022/4/29.
//

#pragma once

#include <nlohmann/json_fwd.hpp>
#include <nlohmann/json.hpp>

#include <variant>
#include <optional>
#include <doodle_core/json_rpc/exception/json_rpc_error.h>
#include <json_rpc/core/rpc_reply.h>

namespace doodle::json_rpc {

using namespace std::literals;
class rpc_request {
  class identifier {
    identifier() = default;

    std::atomic_uint64_t id_;

   public:
    ~identifier() = default;
    static identifier& get();
    std::uint64_t id();
  };

 private:
  constexpr const static auto jsonrpc_version = "2.0";
  friend void to_json(nlohmann::json& nlohmann_json_j, const rpc_request& nlohmann_json_t) {
    nlohmann_json_j["jsonrpc"] = jsonrpc_version;
    nlohmann_json_j["method"]  = nlohmann_json_t.method_;
    if (!nlohmann_json_t.is_notice)
      nlohmann_json_j["id"] = identifier::get().id();
    if (nlohmann_json_t.params_)
      nlohmann_json_j["params"] = *nlohmann_json_t.params_;
  }
  friend void from_json(const nlohmann::json& nlohmann_json_j, rpc_request& nlohmann_json_t) {
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

 public:
  bool is_notice{};
  std::string jsonrpc_{};
  std::string method_{};
  std::optional<nlohmann::json> params_{};
  std::variant<std::int64_t, std::string, std::monostate> id_{0};
};
class rpc_server;
class rpc_server_ref;
class parser_rpc {
 private:
  std::string json_data_{};

  static std::optional<rpc_reply> call_one(
      const rpc_request& in_request,
      const rpc_server_ref& in_server);

 public:
  explicit parser_rpc(std::string string)
      : json_data_(std::move(string)) {}

  std::string operator()(const rpc_server_ref& in_server);
};
}  // namespace doodle::json_rpc
