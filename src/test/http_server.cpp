//
// Created by td_main on 2023/8/16.
//
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/app/program_options.h>

#include "doodle_lib/render_farm/udp_server.h"
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/render_farm/client.h>
#include <doodle_lib/render_farm/detail/computer_manage.h>
#include <doodle_lib/render_farm/detail/ue_task_manage.h>
#include <doodle_lib/render_farm/detail/url_route_base.h>
#include <doodle_lib/render_farm/detail/url_route_get.h>
#include <doodle_lib/render_farm/detail/url_route_post.h>
#include <doodle_lib/render_farm/detail/url_route_put.h>
#include <doodle_lib/render_farm/detail/url_webscoket.h>
#include <doodle_lib/render_farm/udp_server.h>
#include <doodle_lib/render_farm/work.h>
#include <doodle_lib/render_farm/working_machine.h>

#include "boost/asio/executor_work_guard.hpp"
#include <boost/asio.hpp>

class ue_exe_m : public doodle::ue_exe {
 public:
 protected:
  void queue_up(
      const entt::handle& in_msg, const std::string& in_command_line,
      const std::shared_ptr<ue_exe::call_fun_type>& in_call_fun
  ) override {
    DOODLE_LOG_INFO("{}", in_command_line);
    boost::asio::post(doodle::g_io_context(), [in_call_fun, in_command_line]() {
      DOODLE_LOG_INFO("{}", in_command_line);
      (*in_call_fun)(boost::system::error_code{});
    });
  }
};

void run_server() {
  using namespace doodle;
  //  g_ctx().emplace<ue_exe_ptr>() = std::make_shared<ue_exe_m>();
  g_ctx().emplace<doodle::http_listener>(g_io_context()).run();

  g_reg()->ctx().emplace<render_farm::ue_task_manage>().run();
  g_reg()->ctx().emplace<render_farm::computer_manage>().run();

  render_farm::detail::reg_work_websocket{}();
  render_farm::detail::reg_server_websocket{}();

  g_ctx().emplace<doodle::udp_server_ptr>(std::make_shared<udp_server>(g_io_context()))->run();
  auto l_w = g_ctx().emplace<render_farm::work_ptr>(std::make_shared<render_farm::work>());
  l_w->run("192.168.20.59"s);
  app_base::Get().on_stop.connect([=]() {
    g_ctx().get<doodle::render_farm::http_listener>().stop();
    g_ctx().get<render_farm::work_ptr>()->stop();
  });
  //    g_reg()->ctx().emplace<client>("192.168.20.59").run();
}

void stop_work() {
  using namespace doodle;
  //  g_ctx().get<doodle::render_farm::http_listener>().stop();
  //  l_w->stop();
  g_ctx().get<render_farm::work_ptr>()->stop();
}

#define INJECT(name, time, fun)                                  \
  void name() {                                                  \
    using namespace doodle;                                      \
    using timer_t = boost::asio::steady_timer;                   \
    auto l_ptr    = std::make_shared<timer_t>(g_io_context());   \
    l_ptr->expires_after(std::chrono::seconds(time));            \
    l_ptr->async_wait([l_ptr](boost::system::error_code in_ec) { \
      if (in_ec) {                                               \
        DOODLE_LOG_ERROR("{}", in_ec.message());                 \
        return;                                                  \
      }                                                          \
      fun();                                                     \
    });                                                          \
  }

INJECT(inject1, 1, run_server)
INJECT(inject2, 2, stop_work)

#include <doodle_lib/facet/main_facet.h>

#include <iostream>

// #include <doodle_lib/DoodleApp.h>
// #include <boost/locale.hpp>

// extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR strCmdLine, int nCmdShow) try {
int main(int argc, char* argv[]) try {
  using main_app = doodle::app_command<doodle::main_facet>;
  main_app app{argc, argv};
  try {
    inject1();
    //    inject2();
    return app.run();
  } catch (const std::exception& err) {
    DOODLE_LOG_WARN(boost::diagnostic_information(err));
    return 1;
  } catch (...) {
    DOODLE_LOG_ERROR(boost::current_exception_diagnostic_information(true));
    return 1;
  }
  return 0;
} catch (...) {
  std::cout << boost::current_exception_diagnostic_information(true) << std::endl;
  return 1;
}
