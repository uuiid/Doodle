//
// Created by TD on 25-2-17.
//

#include "sid_data.h"

#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/socket_io/engine_io.h>
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
  auto l_sig                             = std::make_shared<boost::asio::cancellation_signal>();
  // auto l_use_awaitable = boost::asio::bind_cancellation_slot(*l_sig, boost::asio::use_awaitable);
  auto l_data                            = std::make_shared<std::string>();
  boost::signals2::scoped_connection l_s = ctx_->on_emit([l_sig, l_data](const socket_io_packet_ptr& in_data) {
    l_sig->emit(boost::asio::cancellation_type::all);
    *l_data = in_data->dump();
  });
  try {
    co_await l_timer.async_wait(boost::asio::bind_cancellation_slot(l_sig->slot(), boost::asio::use_awaitable));
  } catch (const boost::system::system_error& e) {
    if (e.code() != boost::asio::error::operation_aborted) default_logger_raw()->log(log_loc(), level::err, e.what());
  }
  co_return l_data->empty() ? dump_message({}, close_ ? engine_io_packet_type::noop : engine_io_packet_type::ping)
                            : *l_data;
}
}  // namespace doodle::socket_io