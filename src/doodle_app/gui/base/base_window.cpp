//
// Created by TD on 2022/4/8.
//

#include "base_window.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/init_register.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/app/facet/gui_facet.h>

namespace doodle::gui::detail {}
doodle::gui::windows_manage& doodle::gui::g_windows_manage() { return doodle_lib::Get().ctx().get<windows_manage>(); }
void doodle::gui::windows_manage::create_windows_(doodle::gui::windows&& in_windows) {
  if (gui_facet.get().is_render_tick())
    gui_facet.get().windows_list_next.emplace_back(std::move(in_windows));
  else
    gui_facet.get().windows_list.emplace_back(std::move(in_windows));
}

// namespace doodle::gui::detail
