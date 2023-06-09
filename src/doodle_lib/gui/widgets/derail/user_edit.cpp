//
// Created by td_main on 2023/6/9.
//

#include "user_edit.h"

#include <doodle_core/metadata/user.h>

#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/lib_warp/imgui_warp.h>

#include "entt/entity/fwd.hpp"
#include "imgui_stdlib.h"
#include <string>
#include <utility>
namespace doodle::gui::render {

bool user_edit(const entt::handle& in_handle_view) {
  using gui_data = gui_cache_name_id_temp<user_edit_t>;

  bool on_change{false};
  if (!in_handle_view.all_of<user>()) return on_change;

  auto& l_button = in_handle_view.get_or_emplace<gui_data>("用户:"s);
  auto& l_user   = in_handle_view.get<user>();
  if (ImGui::InputText(*l_button, &l_user.get_name())) {
    in_handle_view.patch<user>();
    on_change = true;
  }
  return on_change;
  //  ImGui::Checkbox(*ptr->advanced, &ptr->advanced);
  //  if (ptr->advanced()) {
  //    dear::Text("直接设置用户姓名"s);
  //    if (ImGui::InputText(
  //            *ptr->user_name_edit, &ptr->user_name_edit, ImGuiInputTextFlags_::ImGuiInputTextFlags_EnterReturnsTrue
  //        )) {
  //      if (auto l_user = user::find_by_user_name(ptr->user_name_edit()); l_user) {
  //        ptr->user_tmp_handle = l_user;
  //      } else {
  //        ptr->user_tmp_handle = make_handle();
  //        ptr->user_tmp_handle.get_or_emplace<user>().set_name(ptr->user_name_edit());
  //      }
  //
  //      ptr->set_handle = ptr->user_tmp_handle;
  //      set_modify(true);
  //    }
  //  }
}
std::tuple<bool, entt::handle> select_all_user(const std::string& in_current_user, const registry_ptr& in_reg_ptr) {
  auto& l_list = in_reg_ptr->ctx().get<select_all_user_t>();

  std::tuple<bool, entt::handle> l_ret{false, {}};
  if (dear::Combo const l_com{*l_list.user_id, in_current_user.data()}) {
    l_list.refresh(in_reg_ptr);
    for (auto& i : l_list.user_list) {
      if (imgui::Selectable(i.first.data())) {
        l_ret = {true, i.second};
      }
    }
  }
  return l_ret;
}
std::tuple<bool, entt::handle> select_all_user(const entt::handle& in_current_user, const registry_ptr& in_reg_ptr) {
  return select_all_user(
      in_current_user && in_current_user.all_of<user>() ? in_current_user.get<user>().get_name() : std::string{},
      in_reg_ptr
  );
}

void select_all_user_t::refresh(const registry_ptr& in_reg_ptr) {
  auto l_list = in_reg_ptr->view<user>().each();
  user_list   = l_list |
              ranges::views::transform(
                  [&](const decltype(l_list)::value_type& in_handle) -> std::pair<std::string, entt::handle> {
                    const auto& [l_e, l_user] = in_handle;
                    return std::make_pair(l_user.get_name(), entt::handle{*in_reg_ptr, l_e});
                  }
              ) |
              ranges::to_vector;
}
}  // namespace doodle::gui::render