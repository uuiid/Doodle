//
// Created by TD on 2022/4/29.
//

#pragma once

#include <nlohmann/json_fwd.hpp>
#include <nlohmann/json.hpp>

#include <variant>
#include <optional>
#include <doodle_core/json_rpc/exception/json_rpc_error.h>
#include <doodle_core/json_rpc/core/rpc_reply.h>

#include <boost/signals2.hpp>

#include <boost/asio/spawn.hpp>

namespace doodle::json_rpc {

class rpc_request;
class rpc_server;
class rpc_server_ref;

class parser_rpc {
 private:
  std::string json_data_{};
  bool is_close{false};

 public:
  parser_rpc() = default;
  explicit parser_rpc(std::string string)
      : json_data_(std::move(string)) {}
  void json_data_attr(const std::string& in_string);

  std::string operator()(const rpc_server& in_server);
  [[nodiscard]] explicit operator bool() const;
};
}  // namespace doodle::json_rpc
