//
// Created by TD on 2024/2/20.
//

#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>

#include <nlohmann/json.hpp>
namespace doodle::http {

enum class http_websocket_data_fun {
  ping,
  set_state,
  set_task, logger };
NLOHMANN_JSON_SERIALIZE_ENUM(
    http_websocket_data_fun,
    {
        {http_websocket_data_fun::ping, "ping"},
        {http_websocket_data_fun::set_state, "set_state"},
        {http_websocket_data_fun::set_task, "set_task"},
                              {http_websocket_data_fun::logger, "logger"}}
);

class http_websocket_data {
 public:
  using websocket_stream = boost::beast::websocket::stream<boost::beast::tcp_stream>;
  explicit http_websocket_data(boost::beast::tcp_stream in_stream)
      : stream_(std::make_unique<websocket_stream>(std::move(in_stream))) {}

  std::unique_ptr<websocket_stream> stream_;
  boost::beast::flat_buffer buffer_{};  // (Must persist between reads)

  // read queue
  std::queue<std::string> read_queue_;

  // write queue
  std::queue<std::string> write_queue_;

  void run();
  void do_read();
  void do_write();
  void do_destroy();

  void run_fun();
  // 不一定有回复, 所以不需要回调
  void seed(const nlohmann::json& in_json);
};

}  // namespace doodle::http