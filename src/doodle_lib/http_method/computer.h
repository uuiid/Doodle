//
// Created by TD on 2024/2/26.
//

#pragma once
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>

namespace doodle::http {
enum class computer_websocket_fun { set_state, set_task, logger };

NLOHMANN_JSON_SERIALIZE_ENUM(
    computer_websocket_fun, {{computer_websocket_fun::set_state, "set_state"},
                             {computer_websocket_fun::set_task, "set_task"},
                             {computer_websocket_fun::logger, "logger"}}
);

void computer_reg(http_route& in_route);
}  // namespace doodle::http