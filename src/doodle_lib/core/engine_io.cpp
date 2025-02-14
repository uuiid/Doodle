//
// Created by TD on 25-1-22.
//

#include "engine_io.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/core/core_set.h>

#include <boost/asio/experimental/parallel_group.hpp>

#include "socket_io.h"
#include "socket_io/socket_io_core.h"
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
  return close_ ||
         l_now - last_time_.load() > ctx_->handshake_data_.ping_timeout_ + ctx_->handshake_data_.ping_interval_;
}
void sid_data::update_sid_time() { last_time_ = std::chrono::system_clock::now(); }
void sid_data::close() { close_ = true; }
std::shared_ptr<void> sid_data::get_lock() { return std::make_shared<lock_type>(this); }
bool sid_data::is_locked() const { return lock_count_ > 0; }

void sid_ctx::signal_type::message(const socket_io_packet_ptr& in_data) {
  boost::asio::post(g_io_context(), [in_data, this]() { on_emit_(in_data); });
}

sid_ctx::sid_ctx()
    : handshake_data_{.upgrades_ = {transport_type::websocket}, .ping_interval_ = chrono::milliseconds{250}, .ping_timeout_ = chrono::milliseconds{200}, .max_payload_ = 1000000},
      /// 默认的命名空间
      signal_map_{{std::string{}, std::make_shared<signal_type>()}} {}

void sid_ctx::clear_timeout_sid() {
  std::unique_lock l_lock{mutex_};
  for (auto it = sid_map_.begin(); it != sid_map_.end();) {
    if (it->second->is_timeout()) {
      it = sid_map_.erase(it);
    } else {
      ++it;
    }
  }
}

std::shared_ptr<sid_data> sid_ctx::generate() {
  clear_timeout_sid();
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
sid_ctx::signal_type_ptr sid_ctx::on(const std::string& in_namespace) {
  if (!signal_map_.contains(in_namespace)) signal_map_.emplace(in_namespace, std::make_shared<signal_type>());
  return signal_map_.at(in_namespace);
}

void sid_ctx::emit_connect(const std::shared_ptr<socket_io_core>& in_data) const {
  if (signal_map_.contains(in_data->get_namespace()))
    boost::asio::post(g_io_context(), [in_data, this]() {
      signal_map_.at(in_data->get_namespace())->on_connect_(in_data);
    });
}

void sid_ctx::emit(const socket_io_packet_ptr& in_data) const {
  if (signal_map_.contains(in_data->namespace_))
    boost::asio::post(g_io_context(), [in_data, this]() { signal_map_.at(in_data->namespace_)->on_emit_(in_data); });
}

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