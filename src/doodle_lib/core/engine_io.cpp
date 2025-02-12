//
// Created by TD on 25-1-22.
//

#include "engine_io.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/core/core_set.h>

#include <boost/asio/experimental/parallel_group.hpp>

#include <magic_enum.hpp>
namespace doodle::socket_io {

query_data parse_query_data(const boost::urls::url& in_url) {
  query_data l_ret{.transport_ = transport_type::unknown};
  for (auto&& l_item : in_url.params()) {
    if (l_item.key == "sid" && l_item.has_value) l_ret.sid_ = from_uuid_str(l_item.value);
    if (l_item.key == "transport" && l_item.has_value) {
      l_ret.transport_ = magic_enum::enum_cast<transport_type>(l_item.value).value_or(transport_type::unknown);
    }
    if (l_item.key == "EIO" && l_item.has_value) {
      if (!std::isdigit(l_item.value[0]))
        throw_exception(http_request_error{boost::beast::http::status::bad_request, "无效的请求"});
      l_ret.EIO_ = std::stoi(l_item.value);
    }
  }
  if (l_ret.EIO_ != 4 || l_ret.transport_ == transport_type::unknown)
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "无效的请求"});
  return l_ret;
}
bool sid_data::is_upgrade_to_websocket() const { return is_upgrade_to_websocket_; }
bool sid_data::is_timeout() const {
  auto l_now = std::chrono::system_clock::now();
  return !close_ &&
         l_now - last_time_.load() > ctx_->handshake_data_.ping_timeout_ + ctx_->handshake_data_.ping_interval_;
}
void sid_data::update_sid_time() { last_time_ = std::chrono::system_clock::now(); }
void sid_data::close() { close_ = true; }
std::shared_ptr<void> sid_data::get_lock() { return std::make_shared<lock_type>(this); }
bool sid_data::is_locked() const { return lock_count_ > 0; }

std::shared_ptr<sid_data> sid_ctx::generate() {
  // 加锁
  auto l_ptr = std::make_shared<sid_data>(this);
  std::unique_lock l_lock{mutex_};
  sid_map_.emplace(l_ptr->sid_, l_ptr);
  return l_ptr;
}

std::shared_ptr<sid_data> sid_ctx::get_sid(const uuid& in_sid) const {
  // 加锁
  std::shared_lock l_lock{mutex_};
  return sid_map_.contains(in_sid) ? sid_map_.at(in_sid) : nullptr;
}

boost::asio::awaitable<std::string> sid_data::async_event() {
  boost::asio::system_timer l_timer{co_await boost::asio::this_coro::executor};
  l_timer.expires_at(last_time_.load() + ctx_->handshake_data_.ping_timeout_);
  auto&& [l_array, l_event_ec, l_event, l_ec] =
      co_await boost::asio::experimental::make_parallel_group(
          ctx_->channel_->async_receive(boost::asio::deferred), l_timer.async_wait(boost::asio::deferred)
      )
          .async_wait(boost::asio::experimental::wait_for_one(), boost::asio::use_awaitable);

  switch (l_array[0]) {
    case 0:
      co_return l_event;
    case 1:
      co_return dump_message({}, close_ ? engine_io_packet_type::noop : engine_io_packet_type::ping);
    default:;
  }
}

void sid_ctx::start_run_message_pump() {
  // channel_ = std::make_unique<channel_type>(g_io_context());
  boost::asio::co_spawn(
      g_io_context(), start_run_message_pump_impl(),
      boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
  );
}
boost::asio::awaitable<void> sid_ctx::start_run_message_pump_impl() {
  try {
    boost::asio::system_timer l_timer{co_await boost::asio::this_coro::executor};
    while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
      channel_->try_send(boost::system::error_code{}, dump_message({}, engine_io_packet_type::ping));
      l_timer.expires_from_now(handshake_data_.ping_interval_);
      co_await l_timer.async_wait(boost::asio::use_awaitable);
    }
  } catch (...) {
    default_logger_raw()->error(boost::current_exception_diagnostic_information());
  }
}

}  // namespace doodle::socket_io