//
// Created by TD on 24-7-25.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/core/co_queue.h>

#include <boost/url/url.hpp>

#include "http_route.h"

namespace doodle::http {
class http_websocket_client {
  using executor_type   = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;
  using endpoint_type   = boost::asio::ip::tcp::endpoint;
  using tcp_stream_type = executor_type::as_default_on_t<boost::beast::tcp_stream>;

  std::shared_ptr<boost::beast::websocket::stream<tcp_stream_type>> web_stream_;
  friend boost::asio::awaitable<void> async_write_websocket(std::shared_ptr<http_websocket_client> in_client,
                                                            const std::string& in_data);
  boost::urls::url url_{};
  logger_ptr logger_;
  websocket_route_ptr websocket_route_;
  std::shared_ptr<awaitable_queue_limitation> write_queue_limitation_;

public:
  http_websocket_client()  = default;
  ~http_websocket_client() = default;
  // 客户端使用
  boost::asio::awaitable<void> init(const std::string& in_url, websocket_route_ptr in_websocket_route);
  // 服务端使用
  void init(boost::beast::websocket::stream<tcp_stream_type> in_stream, websocket_route_ptr in_websocket_route,
            logger_ptr in_logger);

  boost::asio::awaitable<void> async_read_websocket();
  boost::asio::awaitable<void> async_write_websocket(std::string in_data);
};
}