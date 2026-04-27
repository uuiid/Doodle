//
// Created by TD on 25-2-17.
//

#include "sid_data.h"

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/core/socket_io/engine_io.h>
#include <doodle_lib/core/socket_io/socket_io_core.h>
#include <doodle_lib/core/socket_io/socket_io_ctx.h>
#include <doodle_lib/core/socket_io/socket_io_packet.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/logger/logger.h>

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/lockfree/detail/uses_optional.hpp>
#include <boost/scope/scope_exit.hpp>

#include "core_enum.h"
#include "websocket_impl.h"
#include <spdlog/spdlog.h>

namespace doodle::socket_io {
using namespace boost::asio::experimental::awaitable_operators;

sid_data::~sid_data() = default;

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
  boost::asio::co_spawn(
      g_io_context(), impl_run(),
      boost::asio::bind_cancellation_slot(
          app_base::Get().on_cancel.slot(), boost::asio::consign(boost::asio::detached, shared_from_this())
      )
  );
}
boost::asio::awaitable<void> sid_data::impl_run() {
  boost::scope::scope_exit l_cleanup{[this, sh = shared_from_this()]() {
    close();
    seed_message_ping();
    ctx_->remove_sid(sid_);
  }};
  boost::asio::system_timer l_timer{co_await boost::asio::this_coro::executor};
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    if (is_timeout()) co_return;
    l_timer.expires_after(ctx_->handshake_data_.ping_interval_);
    auto&& [l_ec] = co_await l_timer.async_wait(boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>{});
    if (l_ec) break;
    if (auto l_websocket = socket_io_websocket_core_.lock(); !l_websocket) co_return;
    seed_message_ping();
  }
  co_return;
}

std::tuple<bool, std::shared_ptr<engine_io_packet>> sid_data::handle_engine_io(std::string& in_data) {
  bool l_ret{true};
  std::shared_ptr<engine_io_packet> l_ptr{};
  switch (auto l_engine_packet = parse_engine_packet(in_data); l_engine_packet) {
    case engine_io_packet_type::open:  // 服务器再get中, 已经处理了open, 不会收到这个消息, 静默
      break;
    case engine_io_packet_type::ping:  // 服务器会在第一次连接websocket时, 收到 ping, 需要回复pong
      update_sid_time();
      in_data.erase(0, 1);
      l_ptr = std::make_shared<engine_io_packet>(engine_io_packet_type::pong, in_data);
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
    case engine_io_packet_type::noop: {
      auto l_ptr = std::make_shared<packet_base>();
      l_ptr->set_data(engine_io_packet{engine_io_packet_type::noop});
      seed_message_self(l_ptr);
    } break;
  }
  return {l_ret, l_ptr};
}

void sid_data::handle_socket_io(socket_io_packet& in_body) {
  if (!ctx_->has_register(in_body.namespace_)) {
    in_body.type_      = socket_io_packet_type::connect_error;
    in_body.json_data_ = nlohmann::json{{"message", "Invalid namespace"}};
    auto l_ptr         = std::make_shared<packet_base>();
    l_ptr->set_data(socket_io_packet{in_body});
    seed_message_self(l_ptr);
    return;
  }

  switch (in_body.type_) {
    case socket_io_packet_type::connect: {
      auto l_ptr                              = std::make_shared<socket_io_core>(ctx_, shared_from_this());
      socket_io_contexts_[in_body.namespace_] = l_ptr;
      l_ptr->set_namespace(in_body.namespace_, in_body.json_data_);
      socket_io_packet l_p{};
      l_p.type_      = socket_io_packet_type::connect;
      l_p.namespace_ = in_body.namespace_;
      l_p.json_data_ = nlohmann::json{{"sid", l_ptr->get_sid()}};
      auto l_packet  = std::make_shared<packet_base>();
      l_packet->set_data(socket_io_packet{l_p});
      seed_message_self(l_packet);
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
  message_queue_.push(in_message);
  write_websocket();
}
void sid_data::seed_message_self(const std::shared_ptr<packet_base>& in_message) {
  if (block_message_) return;
  if (!in_message) return;
  self_message_queue_.push(in_message);
  write_websocket();
}
void sid_data::seed_message_ping() {
  auto l_ptr = std::make_shared<packet_base>();
  l_ptr->set_data(engine_io_packet{engine_io_packet_type::ping});
  ping_message_.write(l_ptr);
  write_websocket();
}
void sid_data::write_websocket() {
  if (auto l_websocket = socket_io_websocket_core_.lock(); l_websocket) l_websocket->write_msg();
}
packet_base_ptr sid_data::get_message() {
  if (auto l_ping_message_ = ping_message_.read(boost::lockfree::uses_optional); l_ping_message_) {
    auto l_ptr = *l_ping_message_;
    return l_ptr;
  }
  std::shared_ptr<packet_base> l_msg{};
  if (self_message_queue_.pop(l_msg); l_msg) return l_msg;
  if (message_queue_.try_pop(l_msg) && l_msg) return l_msg;
  return nullptr;
}
}  // namespace doodle::socket_io