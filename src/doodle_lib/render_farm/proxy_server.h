//
// Created by td_main on 2023/8/18.
//
#pragma once
#include <doodle_core/core/global_function.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <memory>
namespace doodle {
class proxy_server {
 public:
  explicit proxy_server(
      boost::asio::io_context& in_io_context, std::uint16_t in_port, boost::beast::tcp_stream& in_server_stream_
  )
      : end_point_{boost::asio::ip::tcp::v4(), in_port},
        acceptor_{in_io_context, end_point_},
        server_stream_(in_server_stream_) {}
  void run();
  void stop();

 private:
  void do_accept();
  void on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket);
  boost::asio::ip::tcp::endpoint end_point_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::beast::tcp_stream& server_stream_;
};

}  // namespace doodle
