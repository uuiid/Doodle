//
// Created by td_main on 2023/6/9.
//

#pragma once
#include "doodle_core/doodle_core_fwd.h"

#include "entt/entt.hpp"
namespace doodle::gui::render {
struct command_edit_t {};
bool command_edit(const entt::handle& in_handle_view);

}  // namespace doodle::gui::render
