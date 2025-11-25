//
// Created by TD on 25-2-17.
//

#include "broadcast.h"

#include <doodle_lib/core/socket_io/socket_io_ctx.h>
#include <doodle_lib/core/socket_io/socket_io_packet.h>
#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/core/authorization.h>
namespace doodle::socket_io {
void broadcast(
    const std::string& in_event, const nlohmann::json& in_data, const std::string& in_namespace,
    const std::shared_ptr<sid_ctx>& in_ctx
) {
  if(!g_ctx().contains<authorization>()) return; // 暂时屏蔽服务器广播功能
  sid_ctx* l_sid = in_ctx.get();
  if (!l_sid) l_sid = g_ctx().find<sid_ctx>();

  if (!l_sid) return;

  auto l_ptr        = std::make_shared<socket_io_packet>();
  l_ptr->type_      = socket_io_packet_type::event;
  l_ptr->namespace_ = in_namespace;
  if (in_data.is_array()) {
    l_ptr->json_data_ = in_data;
    l_ptr->json_data_.insert(l_ptr->json_data_.begin(), in_event);
  } else {
    l_ptr->json_data_ = nlohmann::json::array();
    l_ptr->json_data_.emplace_back(in_event);
    l_ptr->json_data_.emplace_back(in_data);
  }
  l_sid->emit(l_ptr);
}
}  // namespace doodle::socket_io