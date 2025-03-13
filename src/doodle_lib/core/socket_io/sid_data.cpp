//
// Created by TD on 25-2-17.
//

#include "sid_data.h"

#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/socket_io/engine_io.h>
#include <doodle_lib/core/socket_io/socket_io_core.h>
#include <doodle_lib/core/socket_io/socket_io_ctx.h>
#include <doodle_lib/core/socket_io/socket_io_packet.h>

namespace doodle::socket_io {
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

boost::asio::awaitable<std::string> sid_data::async_event() {
  boost::asio::system_timer l_timer{co_await boost::asio::this_coro::executor};
  l_timer.expires_at(last_time_.load() + ctx_->handshake_data_.ping_timeout_);
  auto l_sig                              = std::make_shared<boost::asio::cancellation_signal>();
  // auto l_use_awaitable = boost::asio::bind_cancellation_slot(*l_sig, boost::asio::use_awaitable);
  auto l_data                             = std::make_shared<std::string>();
  boost::signals2::scoped_connection l_s  = ctx_->on_emit([l_sig, l_data](const socket_io_packet_ptr& in_data) {
    l_sig->emit(boost::asio::cancellation_type::all);
    *l_data = in_data->dump();
  });

  boost::signals2::scoped_connection l_s2 = socket_io_signal_.connect([l_sig, l_data](const std::string& in_str) {
    *l_data = in_str;
    l_sig->emit(boost::asio::cancellation_type::all);
  });
  try {
    co_await l_timer.async_wait(boost::asio::bind_cancellation_slot(l_sig->slot(), boost::asio::use_awaitable));
  } catch (const boost::system::system_error& e) {
    if (e.code() != boost::asio::error::operation_aborted) default_logger_raw()->log(log_loc(), level::err, e.what());
  }
  co_return l_data->empty() ? dump_message({}, close_ ? engine_io_packet_type::noop : engine_io_packet_type::ping)
                            : *l_data;
}
void sid_data::set_websocket_connect(const socket_io_websocket_core_ptr& in_websocket) {
  is_upgrade_to_websocket_ = true;
  for (auto& l_value : socket_io_contexts_ | std::views::values) {
    l_value->set_websocket(in_websocket);
  }
}
std::string sid_data::connect_namespace(const std::string& in_namespace) {
  auto l_ptr                        = std::make_shared<socket_io_core>(ctx_, in_namespace, nlohmann::json{});
  socket_io_contexts_[in_namespace] = l_ptr;
  ctx_->emit_connect(l_ptr);
  socket_io_packet l_p{};
  l_p.type_      = socket_io_packet_type::connect;
  l_p.namespace_ = in_namespace;
  l_p.json_data_ = nlohmann::json{{"sid", l_ptr->get_sid()}};
  auto l_str     = l_p.dump();
  socket_io_signal_(l_p.dump());
  return l_str;
}

std::string sid_data::parse_socket_io(socket_io_packet& in_body) {
  if (!ctx_->has_register(in_body.namespace_)) {
    in_body.type_      = socket_io_packet_type::connect_error;
    in_body.json_data_ = nlohmann::json{{"message", "Invalid namespace"}};
    auto l_str         = in_body.dump();
    socket_io_signal_(l_str);
    return l_str;
  }

  switch (in_body.type_) {
    case socket_io_packet_type::connect: {
      auto l_ptr = std::make_shared<socket_io_core>(ctx_, in_body.namespace_, in_body.json_data_);
      socket_io_contexts_[in_body.namespace_] = l_ptr;
      ctx_->emit_connect(l_ptr);
      in_body.json_data_ = nlohmann::json{{"sid", l_ptr->get_sid()}};
      auto l_str         = in_body.dump();
      socket_io_signal_(l_str);
      return l_str;
      break;
    }
    case socket_io_packet_type::disconnect:
      if (socket_io_contexts_.contains(in_body.namespace_)) {
        in_body.type_ = socket_io_packet_type::event;
        auto l_ptr    = socket_io_contexts_[in_body.namespace_];
        if (!in_body.namespace_.empty()) {
          // 转移到主名称空间
          l_ptr->set_namespace({});
        } else
          socket_io_contexts_.erase(in_body.namespace_);
      }
      break;
    case socket_io_packet_type::event:
    case socket_io_packet_type::binary_event:
      ctx_->on(in_body.namespace_)->message(std::make_shared<socket_io_packet>(std::move(in_body)));
      break;
    case socket_io_packet_type::ack:
      break;
    case socket_io_packet_type::connect_error:
      break;
    case socket_io_packet_type::binary_ack:
      break;
  }
  return {};
}
}  // namespace doodle::socket_io