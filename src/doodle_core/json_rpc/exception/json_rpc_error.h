//
// Created by TD on 2022/4/29.
//

#pragma once

#include <doodle_core/exception/exception.h>

#include <exception>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

using namespace std::literals;

namespace doodle::json_rpc {
/**
 * @brief  所有的rpc 错误基类
 *
 */
class rpc_error_exception : public doodle_error {
 public:
  const std::int64_t code{};
  const std::string message{};
  const std::string data{};
  rpc_error_exception(const std::int64_t& in_code, const std::string& in_msg, const std::string& in_data = {})
      : doodle_error(fmt::format("error code {}, msg {}, {}", in_code, in_msg, in_data)),
        code(in_code),
        message(in_msg),
        data(in_data) {}
};
/**
 * @brief json解析错误
 *
 */
class parse_error_exception : public rpc_error_exception {
 public:
  parse_error_exception()
      : rpc_error_exception(
            -32700, "Parse error语法解析错误"s, "服务端接收到无效的json 该错误发送于服务器尝试解析json文本"s
        ) {}
};
class invalid_request_exception : public rpc_error_exception {
 public:
  invalid_request_exception()
      : rpc_error_exception(-32600, "Invalid Request无效请求"s, "发送的json不是一个有效的请求对象"s) {}
};
class method_not_found_exception : public rpc_error_exception {
 public:
  method_not_found_exception() : rpc_error_exception(-32601, "Method not found找不到方法"s, "该方法不存在或无效"s) {}
};
class invalid_params_exception : public rpc_error_exception {
 public:
  invalid_params_exception() : rpc_error_exception(-32602, "Invalid params无效的参数"s, "无效的方法参数"s) {}
};
class internal_error_exception : public rpc_error_exception {
 public:
  internal_error_exception() : rpc_error_exception(-32603, "Internal error内部错误"s, "JSON-RPC内部错误"s) {}
};

class invalid_handle_exception : public rpc_error_exception {
 public:
  invalid_handle_exception() : rpc_error_exception(-32000, "无效的句柄"s, "传入的句柄无效"s) {}
};

class rpc_error {
 private:
  friend void to_json(nlohmann::json& nlohmann_json_j, const rpc_error& nlohmann_json_t) {
    nlohmann_json_j["code"]    = nlohmann_json_t.code;
    nlohmann_json_j["message"] = nlohmann_json_t.message;
    nlohmann_json_j["data"]    = nlohmann_json_t.data;
  }
  friend void from_json(const nlohmann::json& nlohmann_json_j, rpc_error& nlohmann_json_t) {
    nlohmann_json_j.at("code").get_to(nlohmann_json_t.code);
    nlohmann_json_j.at("message").get_to(nlohmann_json_t.message);
    nlohmann_json_j.at("data").get_to(nlohmann_json_t.data);
  }

 public:
  constexpr rpc_error() = default;

  explicit rpc_error(std::int64_t in_code, std::string in_message, std::string in_data)
      : code(in_code), message(std::move(in_message)), data(std::move(in_data)) {}

  template <typename Error, std::enable_if_t<std::is_base_of_v<rpc_error_exception, Error>, bool> = true>
  explicit rpc_error(const Error& in_err) : code(in_err.code), message(in_err.message), data(in_err.data){};

  std::int64_t code{};
  std::string message{};
  std::string data{};

  [[noreturn]] void to_throw() const {
    switch (code) {
      case -32700:
        throw parse_error_exception{};
      case -32600:
        throw invalid_request_exception{};
      case -32601:
        throw method_not_found_exception{};
      case -32602:
        throw invalid_params_exception{};
      case -32603:
        throw internal_error_exception{};
      default:
        throw rpc_error_exception{code, message, data};
    }
  }
};
inline const static rpc_error parse_error{
    -32700, "Parse error语法解析错误"s, "服务端接收到无效的json 该错误发送于服务器尝试解析json文本"s};
inline const static rpc_error invalid_request{-32600, "Invalid Request无效请求"s, "发送的json不是一个有效的请求对象"s};
inline const static rpc_error method_not_found{-32601, "Method not found找不到方法"s, "该方法不存在或无效"s};
inline const static rpc_error invalid_params{-32602, "Invalid params无效的参数"s, "无效的方法参数"s};
inline const static rpc_error internal_error{-32603, "Internal error内部错误"s, "JSON-RPC内部错误"s};

}  // namespace doodle::json_rpc
