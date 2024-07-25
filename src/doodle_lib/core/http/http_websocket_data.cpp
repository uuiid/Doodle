//
// Created by TD on 2024/2/20.
//

#include "http_websocket_data.h"

#include "doodle_core/lib_warp/boost_fmt_asio.h"
#include "doodle_core/lib_warp/boost_fmt_error.h"
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/websocket_route.h>
#include <doodle_lib/doodle_lib_fwd.h>


namespace doodle::http {
http_websocket_data_manager& g_websocket_data_manager() {
  static http_websocket_data_manager l_manager{};
  return l_manager;
}
} // namespace doodle::http