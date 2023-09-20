//
// Created by td_main on 2023/8/10.
//

#include "computer.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/boost_fmt_asio.h>

#include <doodle_lib/core/bind_front_handler.h>
#include <doodle_lib/render_farm/client_core.h>
#include <doodle_lib/render_farm/detail/basic_json_body.h>
#include <doodle_lib/render_farm/detail/ue4_task.h>
#include <doodle_lib/render_farm/websocket.h>

#include <boost/beast.hpp>

#include <magic_enum.hpp>

namespace doodle {
namespace render_farm {

void computer::delay(computer_status in_status) {
  if (chrono::sys_seconds::clock::now() - last_time_ < 1s) {
    return;
  }
  last_time_ = chrono::sys_seconds::clock::now();
  status_    = in_status;
}
void computer::delay(const std::string& in_str) {
  auto l_status = magic_enum::enum_cast<computer_status>(in_str);
  delay(l_status.value_or(computer_status::idle));
}

void computer::run_task(const entt::handle& in_handle) {
  status_            = computer_status::busy;
  last_time_         = chrono::sys_seconds::clock::now() + 10s;
  auto l_self_handle = make_handle(this);

  auto l_web_ptr     = l_self_handle.get<websocket_data>().websocket_ptr_.lock();
  auto l_logger      = l_self_handle.get<socket_logger>().logger_;
  if (!l_web_ptr) {
    return;
  }

  nlohmann::json l_json{};
  l_json["method"]        = "run.ue.render.task";
  l_json["params"]["id"]  = in_handle.entity();
  l_json["params"]["arg"] = in_handle.get<ue4_task>().arg();

  l_web_ptr->async_call(l_json, [in_handle, l_logger](boost::system::error_code ec, const nlohmann::json& in_r) {
    if (ec) {
      log_error(l_logger, fmt::format("任务派发失败: {}", ec));
      in_handle.get<ue4_task>().fail();
      return;
    }
    if (in_r.contains("error")) {
      in_handle.get<ue4_task>().fail();
    }
    if (in_r.contains("result")) {
      DOODLE_LOG_INFO(
          "成功派发任务 {} {}", in_handle.get<ue4_task>().arg().ProjectPath,
          in_handle.get<ue4_task>().arg().out_file_path
      );
    }
  });
}
}  // namespace render_farm
}  // namespace doodle