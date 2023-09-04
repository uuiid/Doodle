//
// Created by td_main on 2023/9/4.
//

#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/signals2.hpp>
namespace doodle {

class udp_client {
 public:
  using socket_t   = boost::asio::ip::udp::socket;
  using ednpoint_t = boost::asio::ip::udp::endpoint;
  using timer_t    = boost::asio::system_timer;

 private:
  struct impl_t {
    explicit impl_t(boost::asio::io_context& in_context) : send_socket_(in_context), timer_(in_context) {}

    socket_t send_socket_;
    timer_t timer_;

    ednpoint_t remove_endpoint_;
    std::uint16_t remove_port_{};
    char recv_buffer_[1024]{};
    std::size_t recv_size_{};
    boost::asio::cancellation_signal cancel_sig_;
  };
  std::unique_ptr<impl_t> ptr_;

  void do_send();
  bool receive_is_server();

 public:
  explicit udp_client(boost::asio::io_context& in_context) : ptr_(std::make_unique<impl_t>(in_context)){};
  ~udp_client() = default;
  template <typename CompletionHandler>
  auto async_find_server(std::uint16_t in_port, CompletionHandler&& in_completion) {
    ptr_->remove_port_ = in_port;
    ptr_->recv_size_   = 0;
    //    ptr_->signal_.disconnect_all_slots();
    //    ptr_->signal_.connect(std::forward<CompletionHandler>(in_completion));
    //    ptr_->cancel_sig_.emit(boost::asio::cancellation_type::total);
    do_send();

    ptr_->send_socket_.async_receive_from(
        boost::asio::buffer(ptr_->recv_buffer_), ptr_->remove_endpoint_,
        boost::asio::bind_cancellation_slot(
            ptr_->cancel_sig_.slot(),
            [this, l_ptr = ptr_.get(), l_fun = std::forward<CompletionHandler>(in_completion)](
                boost::system::error_code ec, std::size_t in_size
            ) {
              if (!receive_is_server()) {
                BOOST_BEAST_ASSIGN_EC(ec, boost::asio::error::invalid_argument);
              }

              l_ptr->timer_.cancel();
              std::memset(l_ptr->recv_buffer_, 0, in_size);
              l_fun(ec, l_ptr->remove_endpoint_);
              //              l_ptr->signal_(ec, l_ptr->remove_endpoint_);
              //              l_ptr->signal_.disconnect_all_slots();
            }
        )
    );
  };

  template <typename CompletionHandler>
  auto async_find_server(CompletionHandler&& in_completion) {
    return async_find_server(50022, std::forward<CompletionHandler>(in_completion));
  }
};

using udp_client_ptr = std::shared_ptr<udp_client>;

}  // namespace doodle
