//
// Created by TD on 2022/5/13.
//

#pragma once
#include <doodle_core/json_rpc/exception/json_rpc_error.h>

#include <atomic>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <variant>

namespace doodle::json_rpc {

namespace detail {
// helper type for the visitor
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

class base_message {
 public:
};

}  // namespace detail

class DOODLE_CORE_API rpc_reply {
 private:
  friend void DOODLE_CORE_API to_json(nlohmann::json& nlohmann_json_j, const rpc_reply& nlohmann_json_t);
  friend void DOODLE_CORE_API from_json(const nlohmann::json& nlohmann_json_j, rpc_reply& nlohmann_json_t);

 public:
  /// @brief 版本字符串
  std::string jsonrpc_{};
  /// @brief rpc结果(成功时包含,失败时包含error)
  std::variant<nlohmann::json, rpc_error> result{};
  /// @brief id(数值, 字符串, 空值等)
  std::variant<std::int64_t, std::string, std::monostate> id_{};
  constexpr static const std::size_t err_index{1};
};

using namespace std::literals;
class DOODLE_CORE_API rpc_request {
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
  friend void DOODLE_CORE_API to_json(nlohmann::json& nlohmann_json_j, const rpc_request& nlohmann_json_t);
  friend void DOODLE_CORE_API from_json(const nlohmann::json& nlohmann_json_j, rpc_request& nlohmann_json_t);

 public:
  /// @brief json rpc 版本
  std::string jsonrpc_{};
  /// @brief 调用方法
  std::string method_{};
  /// @brief 调用参数
  std::optional<nlohmann::json> params_{};
  /// @brief id 没有id则是通知, 否则id可以是数字,字符串,空值
  std::optional<std::variant<std::int64_t, std::string>> id_{0};
};
}  // namespace doodle::json_rpc
