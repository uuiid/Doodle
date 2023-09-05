//
// Created by td_main on 2023/8/16.
//
#include "doodle_lib/render_farm/udp_server.h"

#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_app/app/program_options.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/render_farm/client.h>
#include <doodle_lib/render_farm/detail/computer_manage.h>
#include <doodle_lib/render_farm/detail/ue_task_manage.h>
#include <doodle_lib/render_farm/detail/url_route_base.h>
#include <doodle_lib/render_farm/detail/url_route_get.h>
#include <doodle_lib/render_farm/detail/url_route_post.h>
#include <doodle_lib/render_farm/udp_client.h>
#include <doodle_lib/render_farm/udp_server.h>
#include <doodle_lib/render_farm/work.h>
#include <doodle_lib/render_farm/working_machine.h>

#include "boost/asio/executor_work_guard.hpp"
#include <boost/asio.hpp>

class server_facet {
  static constexpr auto name{"server"};

 public:
  server_facet()  = default;
  ~server_facet() = default;

  bool post() {
    using namespace doodle;
    bool l_r{};
    l_r          = true;
    guard_       = std::make_shared<decltype(guard_)::element_type>(boost::asio::make_work_guard(g_io_context()));

    //    g_ctx().emplace<doodle::udp_server>();
    auto l_udp_c = std::make_shared<udp_client>(g_io_context());
    //    for (auto i = 0; i < 10; i++) {
    l_udp_c->async_find_server([l_udp_c](const boost::system::error_code& in_ec, const udp_client::ednpoint_t& in_ip) {
      if (in_ec) {
        DOODLE_LOG_ERROR("{}", in_ec);
        return;
      }
      DOODLE_LOG_INFO("{}", in_ip.address().to_string());
    });
    //    }
    g_reg()->ctx().emplace<udp_client_ptr>(l_udp_c);

    return l_r;
  }
  void add_program_options() {}

 private:
  std::shared_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> guard_;
};
#include <doodle_lib/facet/main_facet.h>

#include <iostream>

// #include <doodle_lib/DoodleApp.h>
// #include <boost/locale.hpp>

// extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR strCmdLine, int nCmdShow) try {
int udp_server(int argc, char* argv[]) try {
  using main_app = doodle::app_command<server_facet>;
  main_app app{argc, argv};
  try {
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
