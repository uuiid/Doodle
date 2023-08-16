//
// Created by td_main on 2023/8/16.
//
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_app/app/program_options.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/render_farm/detail/url_route_base.h>
#include <doodle_lib/render_farm/detail/url_route_get.h>
#include <doodle_lib/render_farm/detail/url_route_post.h>
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
    auto l_name = doodle_lib::Get().ctx().get<program_options>().arg[name];
    if (l_name) {
      doodle_lib::Get().ctx().get<program_info>().use_gui_attr(false);
      l_r    = true;
      guard_ = std::make_shared<decltype(guard_)::element_type>(boost::asio::make_work_guard(g_io_context()));
      doodle_lib::Get()
          .ctx()
          .emplace<doodle::render_farm::working_machine_ptr>(
              std::make_shared<doodle::render_farm::working_machine>(g_io_context(), 50021)
          )
          ->config_server();
    }
    return l_r;
  }
  void add_program_options() {}

 private:
  std::shared_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> guard_;
};
#include <doodle_lib/facet/main_facet.h>
#include <doodle_lib/facet/server_facet.h>

#include <iostream>

// #include <doodle_lib/DoodleApp.h>
// #include <boost/locale.hpp>

// extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR strCmdLine, int nCmdShow) try {
extern "C" int main(int argc, const char* const argv[]) try {
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
