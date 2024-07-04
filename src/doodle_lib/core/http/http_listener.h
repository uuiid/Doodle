//
// Created by td_main on 2023/8/3.
//
#pragma once
#include "doodle_core/configure/static_value.h"
#include "doodle_core/core/global_function.h"

#include "boost/asio.hpp"
#include "boost/beast.hpp"

#include <cstdint>
#include <memory>
#include <utility>
namespace doodle::http {
class http_route;
using http_route_ptr = std::shared_ptr<http_route>;
class http_listener {
 public:
  using acceptor_type = boost::asio::ip::tcp::acceptor;
  using endpoint_type = boost::asio::ip::tcp::endpoint;
  explicit http_listener(
      boost::asio::io_context& in_io_context, http_route_ptr in_route_ptr,
      std::uint16_t in_port = doodle_config::http_port
  )
      : executor_{in_io_context.get_executor()},
        end_point_{boost::asio::ip::tcp::v4(), in_port},
        route_ptr_{std::move(in_route_ptr)} {}
  template <typename io_executor_t>
  http_listener(
      io_executor_t in_executor, http_route_ptr in_route_ptr, std::uint16_t in_port = doodle_config::http_port
  )
      : executor_{in_executor}, end_point_{boost::asio::ip::tcp::v4(), in_port}, route_ptr_{std::move(in_route_ptr)} {}
  ~http_listener() = default;

  void run();

 private:
  void do_accept();
  void on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket);
  boost::asio::any_io_executor executor_;
  endpoint_type end_point_;
  http_route_ptr route_ptr_;
  std::shared_ptr<acceptor_type> acceptor_ptr_;
};

namespace detail {
boost::asio::awaitable<void> run_http_listener(
    boost::asio::io_context& in_io_context, http_route_ptr in_route_ptr,
    std::uint16_t in_port = doodle_config::http_port
);
}

void run_http_listener(
    boost::asio::io_context& in_io_context, http_route_ptr in_route_ptr,
    std::uint16_t in_port = doodle_config::http_port
);

}  // namespace doodle::http
