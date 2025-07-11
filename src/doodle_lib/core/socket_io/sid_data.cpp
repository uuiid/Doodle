//
// Created by TD on 25-2-17.
//

#include "sid_data.h"

#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/socket_io/engine_io.h>
#include <doodle_lib/core/socket_io/socket_io_core.h>
#include <doodle_lib/core/socket_io/socket_io_ctx.h>
#include <doodle_lib/core/socket_io/socket_io_packet.h>

#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
namespace doodle::socket_io {
using namespace boost::asio::experimental::awaitable_operators;
bool sid_data::is_upgrade_to_websocket() const { return is_upgrade_to_websocket_; }
bool sid_data::is_timeout() const {
  auto l_now = std::chrono::system_clock::now();
  return close_ ||
         l_now - last_time_.load() > ctx_->handshake_data_.ping_timeout_ + ctx_->handshake_data_.ping_interval_;
}
void sid_data::update_sid_time() { last_time_ = std::chrono::system_clock::now(); }
void sid_data::close() { close_ = true; }
std::shared_ptr<void> sid_data::get_lock() { return std::make_shared<lock_type>(this); }
bool sid_data::is_locked() const { return lock_count_ > 0; }

void sid_data::run() {
  boost::asio::co_spawn(g_io_context(), impl_run(), boost::asio::consign(boost::asio::detached, shared_from_this()));
}
boost::asio::awaitable<void> sid_data::impl_run() {
  boost::asio::system_timer l_timer{co_await boost::asio::this_coro::executor};
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    if (is_timeout()) co_return;
    l_timer.expires_from_now(ctx_->handshake_data_.ping_interval_);
    co_await l_timer.async_wait(boost::asio::use_awaitable);
    seed_message(std::make_shared<engine_io_packet>(engine_io_packet_type::ping));
  }
  co_return;
}

boost::asio::awaitable<std::shared_ptr<packet_base>> sid_data::async_event() {
  boost::asio::system_timer l_timer{co_await boost::asio::this_coro::executor};
  l_timer.expires_from_now(ctx_->handshake_data_.ping_timeout_ + ctx_->handshake_data_.ping_interval_);

  std::shared_ptr<packet_base> l_message{std::make_shared<engine_io_packet>(engine_io_packet_type::noop)};
  if (auto [l_arr, l_e1, l_str_var, l_e2] =
          co_await boost::asio::experimental::parallel_group(
              channel_.async_receive(boost::asio::bind_cancellation_slot(channel_signal_.slot(),boost::asio::deferred)), l_timer.async_wait(boost::asio::deferred)
          )
              .async_wait(
                  boost::asio::experimental::wait_for_one(),
                  boost::asio::use_awaitable
              );
      l_arr[0] == 0)
    l_message = l_str_var;
  if (is_timeout()) l_message = std::make_shared<engine_io_packet>(engine_io_packet_type::noop);
  co_return l_message;
}

bool sid_data::handle_engine_io(std::string& in_data) {
  bool l_ret{true};
  switch (auto l_engine_packet = parse_engine_packet(in_data); l_engine_packet) {
    case engine_io_packet_type::open:  // 服务器再get中, 已经处理了open, 不会收到这个消息, 静默
      break;
    case engine_io_packet_type::ping:  // 服务器会在第一次连接websocket时, 收到 ping, 需要回复pong
      update_sid_time();
      in_data.erase(0, 1);
      seed_message(std::make_shared<engine_io_packet>(engine_io_packet_type::pong, in_data));
      break;
    case engine_io_packet_type::pong:  // 收到pong后, 直接返回, 不在消息队列中处理
      update_sid_time();
      break;
    case engine_io_packet_type::message:
      in_data.erase(0, 1);  // 去除 engine_io_packet_type , 剩下的都是socket io消息内容
      l_ret = false;
      break;
    case engine_io_packet_type::close:
      close();
    case engine_io_packet_type::upgrade:
      upgrade_to_websocket();
      cancel_async_event();
    case engine_io_packet_type::noop:
      seed_message(std::make_shared<engine_io_packet>(engine_io_packet_type::noop));
      break;
  }
  return l_ret;
}

void sid_data::handle_socket_io(socket_io_packet& in_body) {
  if (!ctx_->has_register(in_body.namespace_)) {
    in_body.type_      = socket_io_packet_type::connect_error;
    in_body.json_data_ = nlohmann::json{{"message", "Invalid namespace"}};
    seed_message(std::make_shared<socket_io_packet>(in_body));
    return;
  }

  switch (in_body.type_) {
    case socket_io_packet_type::connect: {
      auto l_ptr                              = std::make_shared<socket_io_core>(ctx_, shared_from_this());
      socket_io_contexts_[in_body.namespace_] = l_ptr;
      l_ptr->set_namespace(in_body.namespace_, in_body.json_data_);
      auto l_p        = std::make_shared<socket_io_packet>();
      l_p->type_      = socket_io_packet_type::connect;
      l_p->namespace_ = in_body.namespace_;
      l_p->json_data_ = nlohmann::json{{"sid", l_ptr->get_sid()}};
      seed_message(l_p);
      ctx_->emit_connect(l_ptr);
      break;
    }
    case socket_io_packet_type::disconnect:
      if (socket_io_contexts_.contains(in_body.namespace_)) {
        block_message_guard l_guard{this};
        in_body.type_ = socket_io_packet_type::event;
        auto l_ptr    = socket_io_contexts_[in_body.namespace_];
        if (!in_body.namespace_.empty()) {
          // 转移到主名称空间
          l_ptr->set_namespace({}, in_body.json_data_);
          socket_io_contexts_[""] = l_ptr;
          ctx_->emit_connect(l_ptr);
        } else
          socket_io_contexts_.erase(in_body.namespace_);
      }
      break;
    case socket_io_packet_type::event:
    case socket_io_packet_type::binary_event:
      ctx_->on(in_body.namespace_)->message(std::make_shared<socket_io_packet>(std::move(in_body)));
      break;
    case socket_io_packet_type::ack:
    case socket_io_packet_type::connect_error:
    case socket_io_packet_type::binary_ack:
      break;
  }
}

void sid_data::seed_message(const std::shared_ptr<packet_base>& in_message) {
  if (block_message_) return;
  if (!in_message) return;
  if (in_message->get_dump_data().empty()) in_message->start_dump();
  if (!channel_.try_send(boost::system::error_code{}, in_message))
    channel_.async_send(boost::system::error_code{}, in_message, [](boost::system::error_code ec) {
      if (ec) default_logger_raw()->error("seed_message error {}", ec.message());
    });
}
void sid_data::cancel_async_event() { channel_signal_.emit(boost::asio::cancellation_type::all); }

}  // namespace doodle::socket_io