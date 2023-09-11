//
// Created by td_main on 2023/9/1.
//

#pragma once
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/global_function.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>

#include <memory>
namespace doodle {

class udp_server : std::enable_shared_from_this<udp_server> {
 public:
  explicit udp_server(boost::asio::io_context& in_io_context, std::uint16_t in_port = doodle_config::udp_port)
      : socket_{in_io_context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), in_port)},
        signal_set_{in_io_context, SIGINT, SIGTERM},
        logger_ptr_{g_logger_ctrl().make_log(fmt::format("udp_server {}", fmt::ptr(this)))} {
    signal_set_.async_wait([this](boost::system::error_code ec, int signal) {
      if (ec) {
        log_error(logger_ptr_, fmt::format("signal_set_ error: {}", ec.message()));
        return;
      }
      log_info(logger_ptr_, fmt::format("signal_set_ signal: {}", signal));
      socket_.cancel();
      socket_.close();
    });
  }

  void run();

 private:
  void do_accept(std::size_t in_size);

  boost::asio::ip::udp::endpoint end_point_;
  boost::asio::ip::udp::socket socket_;
  logger_ptr logger_ptr_;
  char buffer_[65535];

  boost::asio::signal_set signal_set_;
};
using udp_server_ptr = std::shared_ptr<udp_server>;

}  // namespace doodle
