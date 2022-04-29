//
// Created by TD on 2022/4/29.
//

#pragma once

#include <nlohmann/json_fwd.hpp>
#include <nlohmann/json.hpp>

#include <variant>
#include <optional>
#include <exception>
using namespace std::literals;
class rpc_request {
 private:
  //  NLOHMANN_DEFINE_TYPE_INTRUSIVE(rpc_request, jsonrpc_);
  friend void to_json(nlohmann::json& nlohmann_json_j, const rpc_request& nlohmann_json_t) {
  }
  friend void from_json(const nlohmann::json& nlohmann_json_j, rpc_request& nlohmann_json_t) {
    nlohmann_json_j.at("jsonrpc").get_to(nlohmann_json_t.jsonrpc_);
    nlohmann_json_j.at("method").get_to(nlohmann_json_t.method_);
    auto&& l_j = nlohmann_json_j.at("id");
    if (l_j.is_number())
      nlohmann_json_t.id_ = l_j.get<std::int64_t>();
    else if (l_j.is_string())
      nlohmann_json_t.id_ = l_j.get<std::string>();
    else if (l_j.is_null())
      nlohmann_json_t.id_ = std::monostate{};
    else
      throw ;

    if (nlohmann_json_j.contains("params"))
      nlohmann_json_t.params_ = nlohmann_json_j.at("params");
  }

 public:
  std::string jsonrpc_{};
  std::string method_{};
  std::optional<nlohmann::json> params_{};
  std::variant<std::int64_t, std::string, std::monostate> id_{0};
};

class rpc_error {
 public:
  constexpr rpc_error() = default;
  explicit rpc_error(std::int64_t in_code,
                     std::string in_message,
                     std::string in_data)
      : code(in_code),
        message(std::move(in_message)),
        data(std::move(in_data)) {}
  std::int64_t code{};
  std::string message{};
  std::string data{};
};
inline const static rpc_error parse_error{-32700, "Parse error语法解析错误"s, "服务端接收到无效的json 该错误发送于服务器尝试解析json文本"s};
inline const static rpc_error invalid_request{-32600, "Invalid Request无效请求"s, "发送的json不是一个有效的请求对象"s};
inline const static rpc_error method_not_found{-32601, "Method not found找不到方法"s, "该方法不存在或无效"s};
inline const static rpc_error invalid_params{-32602, "Invalid params无效的参数"s, "无效的方法参数"s};
inline const static rpc_error internal_error{-32603, "Internal error内部错误"s, "JSON-RPC内部错误"s};

class rpc_reply {
 public:
  std::string jsonrpc_{};
  std::variant<nlohmann::json, rpc_error> result{};
  std::variant<std::int64_t, std::string, std::monostate> id_{};
};

class parser_rpc {
 private:
  std::string json_data_{};

 public:
  explicit parser_rpc(std::string string)
      : json_data_(std::move(string)) {}
};
