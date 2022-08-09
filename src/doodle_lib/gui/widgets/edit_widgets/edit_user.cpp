//
// Created by TD on 2022/8/9.
//

#include "edit_user.h"
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/assets_file.h>
namespace doodle {
namespace gui {

class edit_user::impl {
 public:
  gui_cache_name_id button{"设置user"s};
  entt::handle user_handle;
};

edit_user::edit_user()
    : ptr(std::make_unique<impl>()) {
}
edit_user::~edit_user() = default;

void edit_user::render(const entt::handle& in) {
  if (ImGui::Button(*ptr->button)) {
    set_modify(true);
  }
}
void edit_user::init_(const entt::handle& in) {
  ptr->user_handle = user::get_current_handle();
}
void edit_user::save_(const entt::handle& in) const {
  if (in.any_of<assets_file>()) {
    in.get<assets_file>().user_attr(ptr->user_handle);
  }
}
}  // namespace gui
}  // namespace doodle
