//
// Created by td_main on 2023/6/9.
//

#pragma once
#include "doodle_core/doodle_core_fwd.h"

#include "doodle_app/gui/base/ref_base.h"

#include "entt/entt.hpp"
namespace doodle::gui::render {
class command_edit_t {
  entt::handle render_id{};
  gui_cache_name_id id{"备注:"s};

 public:
  bool render(const entt::handle& in_handle_view);
};

}  // namespace doodle::gui::render
