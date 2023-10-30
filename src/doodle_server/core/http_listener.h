//
// Created by td_main on 2023/8/3.
//
#pragma once
#include "doodle_core/configure/static_value.h"
#include "doodle_core/core/global_function.h"

#include "boost/asio.hpp"
#include "boost/beast.hpp"

#include "doodle_server/doodle_server_fwd.h"
#include <memory>
namespace doodle {

class http_listener {
  class cancellation_signals {
    std::list<boost::asio::cancellation_signal> sigs;
    std::mutex mtx;

   public:
    void emit(boost::asio::cancellation_type ct = boost::asio::cancellation_type::all);

    boost::asio::cancellation_slot slot();
  };

 public:
  using acceptor_type = boost::asio::ip::tcp::acceptor;
  using endpoint_type = boost::asio::ip::tcp::endpoint;
  explicit http_listener(boost::asio::io_context& in_io_context, std::uint16_t in_port = doodle_config::http_port)
      : end_point_{boost::asio::ip::tcp::v4(), in_port}, signal_set_{in_io_context, SIGINT, SIGTERM} {}
  ~http_listener() = default;
  void run();
  void stop();

  inline void route(http_route_ptr in_route_ptr) { route_ptr_ = std::move(in_route_ptr); }

 private:
  void do_accept();
  void on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket);
  http_route_ptr route_ptr_;
  std::shared_ptr<acceptor_type> acceptor_ptr_;
  endpoint_type end_point_;
  boost::asio::signal_set signal_set_;
  cancellation_signals cancellation_signals_;
};
}  // namespace doodle
