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
}
using program_options = entt::locator<details::program_options>;

}  // namespace doodle
