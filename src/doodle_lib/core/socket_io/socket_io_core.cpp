//
// Created by TD on 25-2-12.
//

#include "socket_io_core.h"

#include <doodle_core/core/core_set.h>

#include <doodle_lib/core/engine_io.h>
#include <doodle_lib/core/socket_io/websocket_impl.h>

#include "core/socket_io.h"
namespace doodle::socket_io {
socket_io_core::socket_io_core(
    const std::shared_ptr<sid_ctx>& in_ctx, socket_io_websocket_core_wptr in_websocket, const std::string& in_namespace,
    const nlohmann::json& in_json
)
    : sid_(core_set::get_set().get_uuid()),
      websocket_(std::move(in_websocket)),
      ctx_(in_ctx),
      namespace_(in_namespace),
      auth_(in_json) {
  connect();
}
void socket_io_core::emit(const std::string& in_event, const nlohmann::json& in_data) {
  auto l_ptr        = std::make_shared<socket_io_packet>();
  l_ptr->type_      = socket_io_packet_type::event;
  l_ptr->namespace_ = namespace_;
  if (in_data.is_array()) {
    l_ptr->json_data_ = in_data;
    l_ptr->json_data_.insert(l_ptr->json_data_.begin(), in_event);
  } else {
    l_ptr->json_data_ = nlohmann::json::array();
    l_ptr->json_data_.emplace_back(in_event);
    l_ptr->json_data_.emplace_back(in_data);
  }
  ctx_->emit(l_ptr);
}
void socket_io_core::on_impl(const socket_io_packet_ptr& in_data) {
  current_packet_guard l_guard{in_data, this};
  auto l_json       = in_data->json_data_;
  auto l_event_name = in_data->json_data_.front().get_ref<const std::string&>();
  l_json.erase(0);
  if (signal_map_.contains(l_event_name)) {
    (*signal_map_.at(l_event_name))(l_json);
  }
}
void socket_io_core::ask(const nlohmann::json& in_data) {
  if (current_packet_) {
    auto l_data        = current_packet_;
    l_data->type_      = socket_io_packet_type::ack;
    l_data->json_data_ = in_data;
    if (auto l_websocket = websocket_.lock(); l_websocket)
      boost::asio::co_spawn(
          g_io_context(),
          [l_data, l_websocket]() -> boost::asio::awaitable<void> {
            co_await l_websocket->async_write_websocket(l_data->dump());
          },
          boost::asio::detached
      );
  }
}

void socket_io_core::connect() {
  /// 挂载接收消息槽
  on_message_scoped_connection_ =
      ctx_->on(namespace_)
          ->on_message(sid_ctx::signal_type::message_solt_type{std::bind_front(&socket_io_core::on_impl, this)});

  if (auto l_websocket = websocket_.lock(); l_websocket)
    /// 挂载发送消息槽
    on_emit_scoped_connection_ =
        ctx_->on(namespace_)
            ->on_emit(sid_ctx::signal_type::emit_solt_type{[web_ = websocket_](const socket_io_packet_ptr& in_data) {
                        auto l_websocket_ = web_.lock();
                        if (!l_websocket_) return;
                        boost::asio::co_spawn(
                            g_io_context(),
                            [in_data, l_websocket_]() -> boost::asio::awaitable<void> {
                              co_await l_websocket_->async_write_websocket(in_data->dump());
                            },
                            boost::asio::detached
                        );
                      }}.track_foreign(l_websocket));
}
}  // namespace doodle::socket_io