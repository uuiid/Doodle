//
// Created by TD on 2022/9/29.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_app/configure/doodle_app_export.h>

namespace doodle {
template <typename Facet_Defaute, typename... Facet_>
class app_command_base;

namespace details {
class program_options;
}  // namespace details

namespace gui {

namespace details {
class main_proc_handle;

}
using main_proc_handle = entt::locator<gui::details::main_proc_handle>;
}  // namespace gui

using program_options = entt::locator<details::program_options>;

}  // namespace doodle
