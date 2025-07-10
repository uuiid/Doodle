//
// Created by TD on 25-2-17.
//

#include "socket_io_ctx.h"

#include <doodle_lib/core/socket_io/sid_data.h>
#include <doodle_lib/core/socket_io/socket_io_core.h>
#include <doodle_lib/core/socket_io/socket_io_packet.h>
namespace doodle {
namespace socket_io {

void sid_ctx::signal_type::message(const socket_io_packet_ptr& in_data) {
  boost::asio::post(g_io_context(), [in_data, this]() { on_message_(in_data); });
}

sid_ctx::sid_ctx()
    : signal_map_{{std::string{}, std::make_shared<signal_type>()}},
      /// 默认的命名空间
      handshake_data_{
          .upgrades_      = {transport_type::websocket},
          .ping_interval_ = chrono::milliseconds{250},
          .ping_timeout_  = chrono::milliseconds{200},
          .max_payload_   = 1000000
      } {}

void sid_ctx::clear_timeout_sid() {
  std::unique_lock l_lock{mutex_};
  for (auto it = sid_map_.begin(); it != sid_map_.end();) {
    if (it->second.expired()) {
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
  return sid_map_.contains(in_sid) ? sid_map_.at(in_sid).lock() : nullptr;
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
}  // namespace socket_io
}  // namespace doodle