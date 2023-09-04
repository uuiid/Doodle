//
// Created by td_main on 2023/9/4.
//

#include "udp_client.h"

#include <doodle_core/configure/static_value.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>
namespace doodle {

void udp_client::do_send() {
  ++ptr_->recv_size_;
  ptr_->send_socket_.open(boost::asio::ip::udp::v4());
  ptr_->send_socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
  ptr_->send_socket_.set_option(boost::asio::socket_base::broadcast(true));
  boost::asio::ip::udp::endpoint const l_endpoint{boost::asio::ip::address_v4::broadcast(), ptr_->remove_port_};
  ptr_->send_socket_.send_to(
      boost::asio::buffer(doodle_config::hello_world_doodle.data(), doodle_config::hello_world_doodle.size()),
      l_endpoint
  );

  if (ptr_->recv_size_ > 3) {
    ptr_->timer_.cancel();
    ptr_->signal_(boost::asio::error::make_error_code(boost::asio::error::timed_out), ptr_->remove_endpoint_);
    ptr_->signal_.disconnect_all_slots();
    return;
  }

  ptr_->timer_.expires_after(std::chrono::seconds(1));
  ptr_->timer_.async_wait([this](auto&& PH1) {
    if (PH1) {
      DOODLE_LOG_ERROR("{}", PH1);
      return;
    }
    do_send();
  });
}

}  // namespace doodle