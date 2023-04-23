//
// Created by TD on 2022/8/9.
//

#include "edit_user.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/user.h>

#include <doodle_app/lib_warp/imgui_warp.h>
namespace doodle {
namespace gui {

class edit_user::impl {
 public:
  gui_cache_name_id button{"设置user"s};
  gui_cache<std::string> user_name_edit{"姓名"s, ""s};

  gui_cache<bool> advanced{"高级设置", false};

  entt::handle user_handle;

  entt::handle user_tmp_handle;

  entt::handle set_handle;
};

edit_user::edit_user() : ptr(std::make_unique<impl>()) {}
edit_user::~edit_user() = default;

void edit_user::render(const entt::handle& in) {
  if (ImGui::Button(*ptr->button)) {
    set_modify(true);
    ptr->set_handle = ptr->user_handle;
  }
  ImGui::Checkbox(*ptr->advanced, &ptr->advanced);
  if (ptr->advanced()) {
    dear::Text("直接设置用户姓名"s);
    if (ImGui::InputText(
            *ptr->user_name_edit, &ptr->user_name_edit, ImGuiInputTextFlags_::ImGuiInputTextFlags_EnterReturnsTrue
        )) {
      if (auto l_user = user::find_by_user_name(ptr->user_name_edit()); l_user) {
        ptr->user_tmp_handle = l_user;
      } else {
        ptr->user_tmp_handle = make_handle();
        ptr->user_tmp_handle.get_or_emplace<user>().set_name(ptr->user_name_edit());
      }

      ptr->set_handle = ptr->user_tmp_handle;
      set_modify(true);
    }
  }
}
void edit_user::init_(const entt::handle& in) {
  ptr->user_handle = g_reg()->ctx().get<user::current_user>().get_handle();

  /// \brief 初始化名称
  if (in.any_of<assets_file>()) ptr->user_name_edit = in.get<assets_file>().user_attr().get<user>().get_name();
}
void edit_user::save_(const entt::handle& in) const {
  if (in.any_of<assets_file>()) {
    in.get<assets_file>().user_attr(ptr->set_handle);
  }
}
}  // namespace gui
}  // namespace doodle
