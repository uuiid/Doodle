//
// Created by TD on 25-2-12.
//

#include "socket_io_core.h"

#include <doodle_core/core/core_set.h>

#include <doodle_lib/core/engine_io.h>
namespace doodle::socket_io {
socket_io_core::socket_io_core(const std::shared_ptr<sid_ctx>& in_ctx)
    : sid_(core_set::get_set().get_uuid()), ctx_(in_ctx) {}
void socket_io_core::emit(const std::string& in_event, const nlohmann::json& in_data) {
  ctx_->emit(in_event, in_data.dump());
}
}  // namespace doodle::socket_io