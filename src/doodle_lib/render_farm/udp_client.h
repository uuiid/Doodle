//
// Created by td_main on 2023/9/4.
//

#pragma once
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
namespace doodle {

class udp_client {
  using socket_t   = boost::asio::ip::udp::socket;
  using ednpoint_t = boost::asio::ip::udp::endpoint;
  using timer_t    = boost::asio::system_timer;
  struct impl_t {
    explicit impl_t(boost::asio::io_context& in_context)
        : send_socket_(in_context), recv_socket_(in_context), timer_(in_context) {}

    socket_t send_socket_;
    socket_t recv_socket_;
    timer_t timer_;

    ednpoint_t remove_endpoint_;
    std::uint16_t remove_port_{};
    char recv_buffer_[1024]{};
    std::size_t recv_size_{};
    boost::signals2::signal<void(boost::system::error_code, ednpoint_t)> signal_;
  };
  std::unique_ptr<impl_t> ptr_;

  void do_send();

 public:
  explicit udp_client(boost::asio::io_context& in_context) : ptr_(std::make_unique<impl_t>(in_context)){};
  ~udp_client() = default;
  template <typename CompletionHandler>
  auto async_find_server(std::uint16_t in_port, CompletionHandler&& in_completion) {
    ptr_->remove_port_ = in_port;
    ptr_->recv_size_   = 0;
    ptr_->signal_.connect(std::forward<CompletionHandler>(in_completion));
    do_send();

    ptr_->recv_socket_.async_receive_from(
        boost::asio::buffer(ptr_->recv_buffer_), ptr_->remove_endpoint_,
        [l_ptr = ptr_.get()](boost::system::error_code ec, std::size_t in_size) {
          l_ptr->timer_.cancel();
          std::memset(l_ptr->recv_buffer_, 0, in_size);
          l_ptr->signal_(ec, l_ptr->remove_endpoint_);
          l_ptr->signal_.disconnect_all_slots();
        }
    );
  };
};

}  // namespace doodle
