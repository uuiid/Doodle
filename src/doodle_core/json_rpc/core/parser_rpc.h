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

class rpc_request;
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
