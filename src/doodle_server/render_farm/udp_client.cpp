//
// Created by td_main on 2023/9/4.
//

#include "udp_client.h"

#include <doodle_core/configure/static_value.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>
namespace doodle {

void udp_client::do_send() {
  ++ptr_->retry_count_;
  log_info(ptr_->logger_ptr_, fmt::format("开始第 {} 次发送,广播端口 {} ", ptr_->retry_count_, ptr_->remove_port_));
  if (ptr_->retry_count_ > 3) {
    log_info(ptr_->logger_ptr_, fmt::format("发送次数超过 3 次, 取消发送"));
    ptr_->timer_.cancel();
    ptr_->cancel_sig_.emit(boost::asio::cancellation_type::total);
    return;
  }
  if (!ptr_->send_socket_.is_open()) ptr_->send_socket_.open(boost::asio::ip::udp::v4());
  ptr_->send_socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
  ptr_->send_socket_.set_option(boost::asio::socket_base::broadcast(true));
  boost::asio::ip::udp::endpoint const l_endpoint{boost::asio::ip::address_v4::broadcast(), ptr_->remove_port_};
  ptr_->send_socket_.send_to(
      boost::asio::buffer(doodle_config::hello_world_doodle.data(), doodle_config::hello_world_doodle.size()),
      l_endpoint
  );

  ptr_->timer_.expires_after(std::chrono::seconds(1));
  ptr_->timer_.async_wait([this, logger_ = ptr_->logger_ptr_](auto&& PH1) {
    if (PH1) {
      if (!ptr_->success_) log_error(logger_, fmt::format("{}", PH1));
      return;
    }
    do_send();
  });
}
bool udp_client::receive_is_server() {
  std::string_view l_view{ptr_->recv_buffer_, ptr_->recv_size_};
  return doodle_config::hello_world_doodle_server == l_view;
}

void udp_client::cancel() {
  ptr_->cancel_sig_.emit(boost::asio::cancellation_type::total);
  ptr_->timer_.cancel();
}

}  // namespace doodle