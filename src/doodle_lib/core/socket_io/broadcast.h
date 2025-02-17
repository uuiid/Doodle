//
// Created by TD on 25-2-17.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::socket_io {
class sid_ctx;
void broadcast(
    const std::string& in_event, const nlohmann::json& in_data, const std::string& in_namespace = {},
    const std::shared_ptr<sid_ctx>& in_ctx = nullptr
);

}  // namespace doodle::socket_io