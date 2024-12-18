//
// Created by TD on 2022/2/27.
//

#pragma once

#include <doodle_core/configure/doodle_core_export.h>

#include <entt/entt.hpp>

namespace doodle {
using registry_ptr = std::shared_ptr<entt::registry>;
DOODLE_CORE_API registry_ptr &g_reg();

}  // namespace doodle
