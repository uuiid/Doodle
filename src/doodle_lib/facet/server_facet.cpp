//
// Created by td_main on 2023/8/15.
//

#include "server_facet.h"

#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_app/app/program_options.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include "boost/asio/executor_work_guard.hpp"
namespace doodle {
bool server_facet::post() {
  bool l_r{};
  auto l_name = doodle_lib::Get().ctx().get<program_options>().arg[name];
  if (l_name) {
    doodle_lib::Get().ctx().get<program_info>().use_gui_attr(false);
    l_r    = true;
    guard_ = std::make_shared<decltype(guard_)::element_type>(boost::asio::make_work_guard(g_io_context()));
  }
  return l_r;
}
void server_facet::add_program_options() { /* doodle_lib::Get().ctx().get<program_options>().arg.add_param(name); */
}
}  // namespace doodle