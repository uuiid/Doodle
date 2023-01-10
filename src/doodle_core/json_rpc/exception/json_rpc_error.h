//
// Created by TD on 2022/4/29.
//

#pragma once

#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/enum_template_tool.h>

#include "json_rpc/exception/json_rpc_error.h"
#include <cstdint>
#include <exception>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

using namespace std::literals;

namespace doodle::json_rpc {

enum class rpc_error_enum : std::int32_t {
  /// @brief 语法解析错误
  parse_error              = -32700,
  /// @brief 无效请求
  invalid_request          = -32600,
  /// @brief 找不到方法
  method_not_found         = -32601,
  /// @brief 无效的参数
  invalid_params           = -32602,
  /// @brief 内部错误
  internal_error           = -32603,

  /// @brief 无效的句柄
  invalid_handle           = -32000,
  /// @brief 无效的id
  invalid_id               = -32001,
  /// @brief 缺失组件
  missing_components       = -32002,
  /// @brief 权限不足
  insufficient_permissions = -32003,

};

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

#define DOODLE_JSON_RPC_EXCEPTION(error_enum, msg, data)                                                        \
  class error_enum##_exception : public rpc_error_exception {                                                   \
   public:                                                                                                      \
    error_enum##_exception() : rpc_error_exception(enum_to_num(rpc_error_enum::error_enum), msg##s, data##s) {} \
  };

DOODLE_JSON_RPC_EXCEPTION(
    parse_error, "Parse error语法解析错误", "服务端接收到无效的json 该错误发送于服务器尝试解析json文本"
)
DOODLE_JSON_RPC_EXCEPTION(invalid_request, "Invalid Request无效请求", "发送的json不是一个有效的请求对象")
DOODLE_JSON_RPC_EXCEPTION(method_not_found, "Method not found找不到方法", "该方法不存在或无效")
DOODLE_JSON_RPC_EXCEPTION(invalid_params, "Invalid params无效的参数", "无效的方法参数")
DOODLE_JSON_RPC_EXCEPTION(internal_error, "Internal error内部错误", "JSON-RPC内部错误")
DOODLE_JSON_RPC_EXCEPTION(invalid_handle, "无效的句柄", "传入的句柄无效")
DOODLE_JSON_RPC_EXCEPTION(invalid_id, "无效的id", "传入的id无效")
DOODLE_JSON_RPC_EXCEPTION(missing_components, "缺失组件", "传入的句柄缺失组件")
DOODLE_JSON_RPC_EXCEPTION(insufficient_permissions, "权限不足", "权限不足")

#undef DOODLE_JSON_RPC_EXCEPTION

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
    auto l_enuum = num_to_enum<rpc_error_enum>(code);
#define DOODLE_CASE_RPC(error_enum) \
  case rpc_error_enum::error_enum:  \
    throw_exception(error_enum##_exception{})

    switch (l_enuum) {
      DOODLE_CASE_RPC(parse_error);
      DOODLE_CASE_RPC(invalid_request);
      DOODLE_CASE_RPC(method_not_found);
      DOODLE_CASE_RPC(invalid_params);
      DOODLE_CASE_RPC(internal_error);
      DOODLE_CASE_RPC(invalid_handle);
      DOODLE_CASE_RPC(invalid_id);
      DOODLE_CASE_RPC(missing_components);
      DOODLE_CASE_RPC(insufficient_permissions);
      default:
        throw rpc_error_exception{code, message, data};
    }
#undef DOODLE_CASE_RPC
  }
};
inline const static rpc_error parse_error{
    -32700, "Parse error语法解析错误"s, "服务端接收到无效的json 该错误发送于服务器尝试解析json文本"s};
inline const static rpc_error invalid_request{-32600, "Invalid Request无效请求"s, "发送的json不是一个有效的请求对象"s};
inline const static rpc_error method_not_found{-32601, "Method not found找不到方法"s, "该方法不存在或无效"s};
inline const static rpc_error invalid_params{-32602, "Invalid params无效的参数"s, "无效的方法参数"s};
inline const static rpc_error internal_error{-32603, "Internal error内部错误"s, "JSON-RPC内部错误"s};

}  // namespace doodle::json_rpc
