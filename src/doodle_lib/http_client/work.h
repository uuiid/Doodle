//
// Created by TD on 2024/2/29.
//

#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace doodle::http {
class http_work {
  //  using client_core     = doodle::detail::client_core;
  //  using client_core_ptr = std::shared_ptr<client_core>;
  //  boost::asio::basic_io_object;
  using timer          = boost::asio::high_resolution_timer;
  using timer_ptr      = std::shared_ptr<timer>;

  using response_type  = boost::beast::http::response<boost::beast::http::string_body>;
  using request_type   = boost::beast::http::request<boost::beast::http::string_body>;
  using signal_set     = boost::asio::signal_set;
  using signal_set_ptr = std::shared_ptr<signal_set>;
  //  using websocket_ptr   = std::shared_ptr<websocket>;

 public:
  http_work()  = default;
  ~http_work() = default;
};
}  // namespace doodle::http
