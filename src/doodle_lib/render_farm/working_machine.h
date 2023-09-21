//
// Created by td_main on 2023/8/3.
//
#pragma once
#include <doodle_core/core/global_function.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <memory>
namespace doodle::render_farm {

class working_machine {
 public:
  explicit working_machine(boost::asio::io_context& in_io_context, std::uint16_t in_port = doodle_config::http_port)
      : end_point_{boost::asio::ip::tcp::v4(), in_port},
        acceptor_{in_io_context, end_point_},
        signal_set_{g_io_context(), SIGINT, SIGTERM} {}
  ~working_machine() = default;
  void run();
  void stop();

  inline void route(http_route_ptr in_route_ptr) { route_ptr_ = std::move(in_route_ptr); }

 private:
  friend class working_machine_session;
  void do_accept();
  void on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket);
  http_route_ptr route_ptr_;
  boost::asio::ip::tcp::endpoint end_point_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::signal_set signal_set_{g_io_context(), SIGINT, SIGTERM};
};
}  // namespace doodle::render_farm
