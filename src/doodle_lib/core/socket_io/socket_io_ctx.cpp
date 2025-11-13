//
// Created by TD on 25-2-17.
//

#include "socket_io_ctx.h"

#include "doodle_core/doodle_core_fwd.h"

#include <doodle_lib/core/socket_io/sid_data.h>
#include <doodle_lib/core/socket_io/socket_io_core.h>
#include <doodle_lib/core/socket_io/socket_io_packet.h>

#include <boost/asio/post.hpp>

#include <utility>

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
    if (it->second->is_timeout()) {
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
  sid_map_.emplace(l_ptr->get_sid(), l_ptr);
  DOODLE_TO_SELF();
  co_return l_ptr;
}

boost::asio::awaitable<std::shared_ptr<sid_data>> sid_ctx::get_sid(uuid in_sid) const {
  // 加锁
  auto l_sid = std::move(in_sid);
  DOODLE_TO_EXECUTOR(strand_);
  std::shared_ptr<sid_data> l_ptr{};
  l_ptr = sid_map_.contains(l_sid) ? sid_map_.at(l_sid) : nullptr;
  if (l_ptr->is_timeout()) l_ptr = nullptr;
  DOODLE_TO_SELF();

  co_return l_ptr;
}
boost::asio::awaitable<sid_ctx::signal_type_ptr> sid_ctx::on(std::string in_namespace) {
  auto l_namespace = std::move(in_namespace);
  DOODLE_TO_EXECUTOR(strand_);
  signal_type_ptr l_ptr{};
  if (!signal_map_.contains(l_namespace)) signal_map_.emplace(l_namespace, std::make_shared<signal_type>());

  l_ptr = signal_map_.at(l_namespace);
  DOODLE_TO_SELF();

  co_return l_ptr;
}

void sid_ctx::emit_connect(const std::shared_ptr<socket_io_core>& in_data) const {
  if (!in_data) return;
  boost::asio::post(strand_, [this, in_data]() {
    try {
      if (!signal_map_.contains(in_data->get_namespace())) return;
      signal_map_.at(in_data->get_namespace())->on_connect_(in_data);
    } catch (...) {
      default_logger_raw()->error(boost::current_exception_diagnostic_information());
    }
  });
}

void sid_ctx::emit(const socket_io_packet_ptr& in_data) const {
  if (!in_data) return;
  boost::asio::post(strand_, [this, in_data]() {
    try {
      emit_impl(in_data);
    } catch (...) {
      default_logger_raw()->error(boost::current_exception_diagnostic_information());
    }
  });
}
void sid_ctx::emit_to_sid(const socket_io_packet_ptr& in_data, const uuid& in_sid) const {
  if (!in_data) return;
  boost::asio::post(strand_, [this, in_data, in_sid]() {
    try {
      if (!sid_map_.contains(in_sid)) return;
      if (auto l_ptr = sid_map_.at(in_sid); l_ptr) l_ptr->seed_message(in_data);
    } catch (...) {
      default_logger_raw()->error(boost::current_exception_diagnostic_information());
    }
  });
}
void sid_ctx::emit_impl(const socket_io_packet_ptr& in_data) const {
  if (!signal_map_.contains(in_data->namespace_)) return;
  if (!in_data) return;
  std::vector<std::shared_ptr<sid_data>> l_sid_data{};
  for (auto l_it : sid_map_)
    if (auto l_ptr = l_it.second; l_ptr) l_sid_data.emplace_back(l_ptr);
  in_data->start_dump();
  for (auto& l_ptr : l_sid_data) l_ptr->seed_message(in_data);
}
boost::asio::awaitable<bool> sid_ctx::has_register(std::string in_namespace) const {
  auto l_namespace = std::move(in_namespace);
  DOODLE_TO_EXECUTOR(strand_);
  bool l_ret = signal_map_.contains(l_namespace);
  DOODLE_TO_SELF();
  co_return l_ret;
}
void sid_ctx::register_namespace(const std::string& in_namespace) {
  boost::asio::post(strand_, [this, in_namespace]() {
    if (!signal_map_.contains(in_namespace)) signal_map_.emplace(in_namespace, std::make_shared<signal_type>());
  });
}
}  // namespace socket_io
}  // namespace doodle