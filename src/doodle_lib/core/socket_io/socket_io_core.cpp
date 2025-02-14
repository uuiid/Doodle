//
// Created by TD on 25-2-12.
//

#include "socket_io_core.h"

#include <doodle_core/core/core_set.h>

#include <doodle_lib/core/engine_io.h>

#include "core/socket_io.h"
namespace doodle::socket_io {
socket_io_core::socket_io_core(
    const std::shared_ptr<sid_ctx>& in_ctx, const std::string& in_namespace, const nlohmann::json& in_json
)
    : sid_(core_set::get_set().get_uuid()), ctx_(in_ctx), namespace_(in_namespace), auth_(in_json) {
  connect();
}
void socket_io_core::emit(const std::string& in_event, const nlohmann::json& in_data) {
  auto l_ptr        = std::make_shared<socket_io_packet>();
  l_ptr->type_      = socket_io_packet_type::event;
  l_ptr->namespace_ = namespace_;
  l_ptr->json_data_.emplace_back(in_event);
  l_ptr->json_data_.emplace_back(in_data);
  ctx_->emit(l_ptr);
}
void socket_io_core::on_impl(const socket_io_packet_ptr& in_data) {
  auto l_json = in_data->json_data_;
  l_json.erase(0);
  auto l_event_name = in_data->json_data_.front().get_ref<const std::string&>();
  if (signal_map_.contains(l_event_name)) {
    (*signal_map_.at(l_event_name))(l_json);
  }
}

void socket_io_core::connect() {
  scoped_connection_ = ctx_->on(namespace_)->on_message(std::bind_front(&socket_io_core::on_impl, this));
}
}  // namespace doodle::socket_io