//
// Created by td_main on 2023/8/21.
//

#include "work_facet.h"

#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_app/app/program_options.h>

#include <doodle_lib/render_farm/work.h>
#include <doodle_lib/render_farm/working_machine.h>

#include "boost/asio/executor_work_guard.hpp"

namespace doodle {
bool work_facet::post() {
  bool l_r{};
  auto l_name = doodle_lib::Get().ctx().get<program_options>().arg[name];
  if (l_name) {
    doodle_lib::Get().ctx().get<program_info>().use_gui_attr(false);
    l_r    = true;
    guard_ = std::make_shared<decltype(guard_)::element_type>(boost::asio::make_work_guard(g_io_context()));
    doodle_lib::Get().ctx().emplace<doodle::render_farm::work>(core_set::get_set().server_ip).run();
  }
  return l_r;
}
void work_facet::add_program_options() {}
}  // namespace doodle