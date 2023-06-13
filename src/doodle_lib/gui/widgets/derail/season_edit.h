//
// Created by td_main on 2023/6/9.
//
#pragma once
#include <doodle_app/gui/base/ref_base.h>

#include <entt/entt.hpp>
namespace doodle::gui::render {

class season_edit_t {
  gui_cache_name_id id{"季数"s};
  gui_cache_name_id add{"添加季数"s};

 public:
  bool render(const entt::handle& in_handle_view);
};

}  // namespace doodle::gui::render
