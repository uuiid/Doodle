//
// Created by TD on 25-2-17.
//

#include "socket_io_ctx.h"

#include "doodle_core/doodle_core_fwd.h"

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
      strand_{boost::asio::make_strand(g_io_context())},
      /// 默认的命名空间
      handshake_data_{
          .upgrades_      = {transport_type::websocket},
          .ping_interval_ = chrono::milliseconds{25000},
          .ping_timeout_  = chrono::milliseconds{20000},
          .max_payload_   = 1000000
      } {}

void sid_ctx::clear_timeout_sid() {
  for (auto it = sid_map_.begin(); it != sid_map_.end();) {
    if (it->second.expired()) {
      it = sid_map_.erase(it);
    } else {
      ++it;
    }
  }
}

boost::asio::awaitable<std::shared_ptr<sid_data>> sid_ctx::generate() {
  DOODLE_TO_EXECUTOR(strand_);
  clear_timeout_sid();
  // 加锁
  auto l_ptr = std::make_shared<sid_data>(this);
  l_ptr->run();
  sid_map_.emplace(l_ptr->sid_, l_ptr);
  DOODLE_TO_SELF();
  co_return l_ptr;
}

boost::asio::awaitable<std::shared_ptr<sid_data>> sid_ctx::get_sid(const uuid& in_sid) const {
  // 加锁

  DOODLE_TO_EXECUTOR(strand_);
  std::shared_ptr<sid_data> l_ptr{};
  l_ptr = sid_map_.contains(in_sid) ? sid_map_.at(in_sid).lock() : nullptr;
  DOODLE_TO_SELF();

  co_return l_ptr;
}
boost::asio::awaitable<sid_ctx::signal_type_ptr> sid_ctx::on(const std::string& in_namespace) {
  DOODLE_TO_EXECUTOR(strand_);
  signal_type_ptr l_ptr{};
  if (!signal_map_.contains(in_namespace)) signal_map_.emplace(in_namespace, std::make_shared<signal_type>());

  l_ptr = signal_map_.at(in_namespace);
  DOODLE_TO_SELF();

  co_return l_ptr;
}

boost::asio::awaitable<void> sid_ctx::emit_connect(const std::shared_ptr<socket_io_core>& in_data) const {
  DOODLE_TO_EXECUTOR(strand_);
  if (signal_map_.contains(in_data->get_namespace())) signal_map_.at(in_data->get_namespace())->on_connect_(in_data);
  DOODLE_TO_SELF();
  // boost::asio::post(g_io_context(), [in_data, this]() {});
}

void sid_ctx::emit(const socket_io_packet_ptr& in_data) const {
  boost::asio::co_spawn(strand_, emit_impl(in_data), [](std::exception_ptr in_eptr) {
    try {
      if (in_eptr) std::rethrow_exception(in_eptr);
    } catch (const std::exception& e) {
      default_logger_raw()->error(e.what());
    };
  });
}
boost::asio::awaitable<void> sid_ctx::emit_impl(const socket_io_packet_ptr& in_data) const {
  if (!signal_map_.contains(in_data->namespace_)) co_return;
  if (!in_data) co_return;
  std::vector<std::shared_ptr<sid_data>> l_sid_data{};
  for (auto l_it : sid_map_)
    if (auto l_ptr = l_it.second.lock(); l_ptr) l_sid_data.emplace_back(l_ptr);
  in_data->start_dump();
  for (auto& l_ptr : l_sid_data) l_ptr->seed_message(in_data);
}
boost::asio::awaitable<bool> sid_ctx::has_register(const std::string& in_namespace) const {
  DOODLE_TO_EXECUTOR(strand_);
  bool l_ret = signal_map_.contains(in_namespace);
  DOODLE_TO_SELF();
  co_return l_ret;
}
void sid_ctx::register_namespace(const std::string& in_namespace) {
  boost::asio::post(strand_, [this, in_namespace]() {
    if (!signal_map_.contains(in_namespace))
      signal_map_.emplace(in_namespace, std::make_shared<signal_type>());
  });
}
}  // namespace socket_io
}  // namespace doodle