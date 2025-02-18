//
// Created by TD on 25-2-12.
//

#include "socket_io_core.h"

#include <doodle_core/core/core_set.h>

#include <doodle_lib/core/socket_io/engine_io.h>
#include <doodle_lib/core/socket_io/socket_io_ctx.h>
#include <doodle_lib/core/socket_io/socket_io_packet.h>
#include <doodle_lib/core/socket_io/websocket_impl.h>

#include "core/socket_io.h"
namespace doodle::socket_io {
socket_io_core::socket_io_core(sid_ctx* in_ctx, const std::string& in_namespace, const nlohmann::json& in_json)
    : sid_(core_set::get_set().get_uuid()), ctx_(in_ctx), namespace_(in_namespace), auth_(in_json) {
  connect();
}
void socket_io_core::emit(const std::string& in_event, const nlohmann::json& in_data) const {
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
void socket_io_core::emit(const std::string& in_event, const std::vector<std::string>& in_data) const {
  auto l_ptr        = std::make_shared<socket_io_packet>();
  l_ptr->type_      = socket_io_packet_type::binary_event;
  l_ptr->namespace_ = namespace_;
  l_ptr->json_data_ = nlohmann::json::array();
  l_ptr->json_data_.emplace_back(in_event);
  for (auto i = 0; i < in_data.size(); ++i) {
    l_ptr->json_data_.emplace_back(nlohmann::json::object({{"_placeholder", true}, {"num", i}}));
  }
  l_ptr->binary_data_  = in_data;
  l_ptr->binary_count_ = in_data.size();
  ctx_->emit(l_ptr);
}

void socket_io_core::on_impl(const socket_io_packet_ptr& in_data) {
  current_packet_guard l_guard{in_data, this};
  auto l_json       = in_data->json_data_;
  auto l_event_name = in_data->json_data_.front().get_ref<const std::string&>();
  l_json.erase(0);
  if (signal_map_.contains(l_event_name)) {
    try {
      if (in_data->binary_count_ != 0)
        (*signal_map_.at(l_event_name))(in_data->binary_data_);
      else
        (*signal_map_.at(l_event_name))(l_json);
    } catch (...) {
      default_logger_raw()->error(boost::current_exception_diagnostic_information());
    }
  }
}
void socket_io_core::ask(const nlohmann::json& in_data) const {
  if (!current_packet_) return;

  auto l_data        = std::make_shared<socket_io_packet>();
  l_data->type_      = socket_io_packet_type::ack;
  l_data->id_        = current_packet_->id_;
  l_data->namespace_ = namespace_;
  l_data->json_data_ = in_data;
  if (auto l_websocket = websocket_.lock(); l_websocket)
    boost::asio::co_spawn(
        g_io_context(),
        [l_data, l_websocket]() -> boost::asio::awaitable<void> {
          co_await l_websocket->async_write_websocket(l_data);
        },
        boost::asio::detached
    );
}
void socket_io_core::ask(const std::vector<std::string>& in_data) const {
  if (!current_packet_) return;

  auto l_data        = std::make_shared<socket_io_packet>();
  l_data->id_        = current_packet_->id_;
  l_data->type_      = socket_io_packet_type::binary_ack;
  l_data->namespace_ = namespace_;
  l_data->json_data_ = nlohmann::json::array();
  for (auto i = 0; i < in_data.size(); ++i) {
    l_data->json_data_.emplace_back(nlohmann::json::object({{"_placeholder", true}, {"num", i}}));
  }
  l_data->binary_data_  = in_data;
  l_data->binary_count_ = in_data.size();
  if (auto l_websocket = websocket_.lock(); l_websocket)
    boost::asio::co_spawn(
        g_io_context(),
        [l_data, l_websocket]() -> boost::asio::awaitable<void> {
          co_await l_websocket->async_write_websocket(l_data);
        },
        boost::asio::detached
    );
}
void socket_io_core::connect() {
  /// 挂载接收消息槽
  on_message_scoped_connection_ =
      ctx_->on(namespace_)
          ->on_message(sid_ctx::signal_type::message_solt_type{std::bind_front(&socket_io_core::on_impl, this)});

  connect_websocket();
}
void socket_io_core::set_websocket(const socket_io_websocket_core_ptr& in_websocket) {
  websocket_ = in_websocket;
  connect_websocket();
}
void socket_io_core::connect_websocket() {
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
                              co_await l_websocket_->async_write_websocket(in_data);
                            },
                            boost::asio::detached
                        );
                      }}.track_foreign(l_websocket));
}
}  // namespace doodle::socket_io