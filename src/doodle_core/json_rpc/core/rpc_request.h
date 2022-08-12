//
// Created by TD on 2022/5/13.
//

#pragma once
#include <nlohmann/json_fwd.hpp>

#include <atomic>
#include <variant>
#include <optional>

#include <nlohmann/json.hpp>
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
  friend void to_json(nlohmann::json& nlohmann_json_j, const rpc_request& nlohmann_json_t);
  friend void from_json(const nlohmann::json& nlohmann_json_j, rpc_request& nlohmann_json_t);

 public:
  bool is_notice{};
  std::string jsonrpc_{};
  std::string method_{};
  std::optional<nlohmann::json> params_{};
  std::variant<std::int64_t, std::string, std::monostate> id_{0};
};
}  // namespace doodle::json_rpc
