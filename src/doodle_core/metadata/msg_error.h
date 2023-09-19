//
// Created by td_main on 2023/9/19.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <nlohmann/json.hpp>
#include <string>
namespace doodle {

struct DOODLE_CORE_API msg_error {
  std::int64_t code{};
  std::string message{};
  std::string data{};
  enum rpc_error_enum : std::int32_t {
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

    msg_error()              = default;
    // to json
    friend void DOODLE_CORE_API to_json(nlohmann::json & nlohmann_json_j, const msg_error& nlohmann_json_t){
        nlohmann_json_j["code"] = nlohmann_json_t.code; nlohmann_json_j["message"] = nlohmann_json_t.message;
        nlohmann_json_j["data"]                                                    = nlohmann_json_t.data;};
  // from json
  friend void DOODLE_CORE_API from_json(const nlohmann::json& nlohmann_json_j, msg_error& nlohmann_json_t) {
    nlohmann_json_j["code"].get_to(nlohmann_json_t.code);
    nlohmann_json_j["message"].get_to(nlohmann_json_t.message);
    nlohmann_json_j["data"].get_to(nlohmann_json_t.data);
  };
};

}  // namespace doodle