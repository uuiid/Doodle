//
// Created by TD on 2024/2/20.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/socket_logger.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

namespace doodle::http {
class websocket_route;
using websocket_route_ptr = std::shared_ptr<websocket_route>;

enum class http_websocket_data_fun { ping, set_state, set_task, logger, run_task };

NLOHMANN_JSON_SERIALIZE_ENUM(
  http_websocket_data_fun, {{http_websocket_data_fun::ping, "ping"},
  {http_websocket_data_fun::set_state, "set_state"},
  {http_websocket_data_fun::set_task, "set_task"},
  {http_websocket_data_fun::logger, "logger"},
  {http_websocket_data_fun::run_task, "run_task"}}
);


class http_websocket_data_manager {
public:
  http_websocket_data_manager()  = default;
  ~http_websocket_data_manager() = default;
};

http_websocket_data_manager& g_websocket_data_manager();
} // namespace doodle::http