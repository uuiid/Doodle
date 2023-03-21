//
// Created by TD on 2022/4/8.
//

#include "base_window.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/init_register.h>
namespace doodle::gui::detail {
std::tuple<entt::handle, doodle::gui::detail::windows_render> find_windows(const std::string& in_name) {
  for (auto&& [l_e, l_ptr] : g_reg()->view<doodle::gui::gui_windows>().each()) {
    if (l_ptr->title() == in_name) return std::make_tuple(make_handle(l_e), l_ptr);
  }
  return {};
}

}  // namespace doodle::gui::detail
