//
// Created by td_main on 2023/8/18.
//
#pragma once
#include <doodle_core/core/global_function.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/render_farm/client_core.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <memory>
namespace doodle {
class proxy_server {
 public:
  explicit proxy_server(
      boost::asio::io_context& in_io_context, std::uint16_t in_port, std::string in_server_address
      //      std::uint16_t server_port
  )
      : end_point_{boost::asio::ip::tcp::v4(), in_port},
        acceptor_{in_io_context, end_point_},
        server_address_{std::move(in_server_address)} {
    client_core_ptr_ = std::make_shared<detail::client_core>(server_address_);
  }
  void run();
  void stop();

 private:
  void do_accept();
  void on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket);
  boost::asio::ip::tcp::endpoint end_point_;
  boost::asio::ip::tcp::acceptor acceptor_;
  std::shared_ptr<boost::beast::tcp_stream> server_stream_;
  std::string server_address_;

  std::shared_ptr<detail::client_core> client_core_ptr_{};
};

}  // namespace doodle
