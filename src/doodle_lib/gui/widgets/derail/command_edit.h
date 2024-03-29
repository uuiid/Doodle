//
// Created by td_main on 2023/6/9.
//

#pragma once
#include "doodle_core/doodle_core_fwd.h"

#include "doodle_app/gui/base/ref_base.h"

#include "entt/entt.hpp"
namespace doodle::gui::render {
class command_edit_t {
  gui_cache_name_id id{"备注"s};
  gui_cache_name_id add{"添加备注"s};

 public:
  bool render(const entt::handle& in_handle_view);
};

}  // namespace doodle::gui::render
