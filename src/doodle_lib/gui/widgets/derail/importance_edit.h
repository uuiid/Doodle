//
// Created by td_main on 2023/6/9.
//

#pragma once
#include <doodle_app/gui/base/ref_base.h>
namespace doodle::gui::render {

class importance_edit_t {
  gui_cache_name_id id{"重要性"s};
  gui_cache_name_id add{"添加重要性"s};

 public:
  bool render(const entt::handle& in_handle_view);
};

}  // namespace doodle::gui::render
