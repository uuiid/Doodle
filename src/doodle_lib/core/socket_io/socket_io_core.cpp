//
// Created by TD on 25-2-12.
//

#include "socket_io_core.h"

#include <doodle_core/core/core_set.h>

#include <doodle_lib/core/engine_io.h>

#include "core/socket_io.h"
namespace doodle::socket_io {
socket_io_core::socket_io_core(const std::shared_ptr<sid_ctx>& in_ctx, const std::string& in_namespace)
    : sid_(core_set::get_set().get_uuid()), ctx_(in_ctx), namespace_(in_namespace) {}
void socket_io_core::emit(const std::string& in_event, const nlohmann::json& in_data) {
  auto l_ptr        = std::make_shared<socket_io_packet>();
  l_ptr->type_      = socket_io_packet_type::event;
  l_ptr->namespace_ = namespace_;
  l_ptr->json_data_ = nlohmann::json{{in_event, in_data}};
  ctx_->emit(l_ptr);
}
}  // namespace doodle::socket_io