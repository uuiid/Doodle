//
// Created by td_main on 2023/6/9.
//

#pragma once
#include "doodle_app/gui/base/ref_base.h"
namespace doodle::gui::render {

class episodes_edit_t {
  gui_cache_name_id id{"集数"s};
  gui_cache_name_id add{"添加集数"s};

 public:
  bool render(const entt::handle& in_handle_view);
};

}  // namespace doodle::gui::render
