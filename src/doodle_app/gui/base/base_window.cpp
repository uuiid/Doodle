//
// Created by TD on 2022/4/8.
//

#include "base_window.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/init_register.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/app/facet/gui_facet.h>

namespace doodle::gui {

windows_manage& g_windows_manage() { return doodle_lib::Get().ctx().get<windows_manage>(); }

bool windows_manage::has_windows(const entt::type_info& in_info) {
  return ranges::any_of(gui_facet.get().windows_list_next, [&](windows& i) { return i.type() == in_info; }) ||
         ranges::any_of(gui_facet.get().windows_list, [&](windows& i) { return i.type() == in_info; });
}

void windows_manage::create_windows_(windows&& in_windows) {
  if (gui_facet.get().is_render_tick())
    gui_facet.get().windows_list_next.emplace_back(std::move(in_windows));
  else
    gui_facet.get().windows_list.emplace_back(std::move(in_windows));
}

}  // namespace doodle::gui
