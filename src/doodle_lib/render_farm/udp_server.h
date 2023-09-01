//
// Created by td_main on 2023/9/1.
//

#pragma once
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>

#include <memory>
namespace doodle {

class udp_server : std::enable_shared_from_this<udp_server> {
 public:
  explicit udp_server(boost::asio::io_context& in_io_context, std::uint16_t in_port = 50022)
      : socket_{in_io_context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), in_port)},
        signal_set_{in_io_context, SIGINT, SIGTERM} {}
  udp_server() = default;

  void run();

 private:
  void do_accept();

  boost::asio::ip::udp::endpoint end_point_;
  boost::asio::ip::udp::socket socket_;
  std::string buffer_;

  boost::asio::signal_set signal_set_;
};
using udp_server_ptr = std::shared_ptr<udp_server>;

}  // namespace doodle
