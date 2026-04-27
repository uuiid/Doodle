//
// Created by TD on 25-2-17.
//

#include "socket_io_ctx.h"

#include <doodle_lib/core/global_function.h>
#include <doodle_lib/core/socket_io/sid_data.h>
#include <doodle_lib/core/socket_io/socket_io_core.h>
#include <doodle_lib/core/socket_io/socket_io_packet.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/logger/logger.h>

#include <boost/asio/post.hpp>

#include <spdlog/spdlog.h>
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
          .ping_timeout_  = chrono::milliseconds{40000},
          .max_payload_   = 1000000
      } {
  g_ctx().emplace<sid_ctx&>(*this);
}

std::shared_ptr<sid_data> sid_ctx::generate() {
  auto l_ptr = std::make_shared<sid_data>(this);
  l_ptr->run();
  sid_map_.emplace(l_ptr->get_sid(), l_ptr);
  return l_ptr;
}

std::shared_ptr<sid_data> sid_ctx::get_sid(uuid in_sid) const {
  auto l_sid = std::move(in_sid);
  std::shared_ptr<sid_data> l_ptr{};
  if (sid_map_.contains(l_sid))
    sid_map_.cvisit(l_sid, [&l_ptr](const auto& in_pair) {
      if (auto l_data = in_pair.second; l_data && !l_data->is_timeout()) l_ptr = l_data;
    });

  return l_ptr;
}

void sid_ctx::remove_sid(uuid in_sid) { sid_map_.erase(in_sid); }

sid_ctx::signal_type_ptr sid_ctx::on(std::string in_namespace) {
  auto l_namespace = std::move(in_namespace);
  signal_type_ptr l_ptr{};
  if (!signal_map_.contains(l_namespace)) signal_map_.emplace(l_namespace, std::make_shared<signal_type>());
  signal_map_.cvisit(in_namespace, [&l_ptr](const auto& in_pair) {
    if (auto l_data = in_pair.second; l_data) l_ptr = l_data;
  });

  return l_ptr;
}

void sid_ctx::emit_connect(const std::shared_ptr<socket_io_core>& in_data) const {
  if (!in_data) return;
  boost::asio::post(strand_, [this, in_data]() {
    try {
      if (!signal_map_.contains(in_data->get_namespace())) return;
      signal_map_.visit(in_data->get_namespace(), [&in_data](const auto& in_pair) {
        if (auto l_sig = in_pair.second; l_sig) l_sig->on_connect_(in_data);
      });
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
      auto l_packet = std::make_shared<packet_base>();
      l_packet->set_data(*in_data);
      sid_map_.cvisit(in_sid, [&l_packet](const auto& in_pair) {
        if (auto l_data = in_pair.second; l_data && !l_data->is_timeout()) l_data->seed_message(l_packet);
      });

    } catch (...) {
      default_logger_raw()->error(boost::current_exception_diagnostic_information());
    }
  });
}
void sid_ctx::emit_impl(const socket_io_packet_ptr& in_data) const {
  if (!signal_map_.contains(in_data->namespace_)) return;
  if (!in_data) return;
  std::vector<std::shared_ptr<sid_data>> l_sid_data{};

  sid_map_.cvisit_all([&l_sid_data](const auto& in_pair) {
    if (auto l_data = in_pair.second; l_data && !l_data->is_timeout()) l_sid_data.emplace_back(l_data);
  });

  auto l_packet = std::make_shared<packet_base>();
  l_packet->set_data(*in_data);
  for (auto& l_ptr : l_sid_data) l_ptr->seed_message(l_packet);
}
bool sid_ctx::has_register(std::string in_namespace) const {
  auto l_namespace = std::move(in_namespace);
  bool l_ret       = signal_map_.contains(l_namespace);
  return l_ret;
}
void sid_ctx::register_namespace(const std::string& in_namespace) {
  if (!signal_map_.contains(in_namespace)) signal_map_.emplace(in_namespace, std::make_shared<signal_type>());
}

}  // namespace socket_io
}  // namespace doodle