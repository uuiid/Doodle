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

#include <boost/coroutine2/coroutine.hpp>
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
class rpc_server;
class rpc_server_ref;

class parser_rpc {
 private:
  std::string json_data_{};



  using json_coroutine = boost::coroutines2::coroutine<nlohmann::json>;

 public:
  parser_rpc() = default;
  explicit parser_rpc(std::string string)
      : json_data_(std::move(string)) {}
  void json_data_attr(const std::string& in_string);

  std::string operator()(const rpc_server_ref& in_server);

  void operator()(boost::coroutines2::coroutine<std::string>::push_type& sink,
                  const rpc_server_ref& in_server);
};
}  // namespace doodle::json_rpc
