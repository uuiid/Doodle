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
void select_all_user_t::set_current_user(const std::string& in_current_user) { current_user = in_current_user; }
void select_all_user_t::set_current_user(const entt::handle& in_current_user) {
  if (in_current_user && in_current_user.all_of<user>()) {
    current_user = in_current_user.get<user>().get_name();
  } else {
    current_user = {};
  }
}
std::tuple<bool, entt::handle> select_all_user_t::render(const registry_ptr& in_reg_ptr) {
  std::tuple<bool, entt::handle> l_ret{false, {}};
  if (dear::Combo const l_com{*user_id, current_user.data()}) {
    refresh(in_reg_ptr);
    for (auto& i : user_list) {
      if (imgui::Selectable(i.first.data())) {
        l_ret = {true, i.second};
      }
    }
  }
  return l_ret;
}
bool user_edit_t::render(const entt::handle& in_handle_view) {
  if (!in_handle_view.all_of<user>()) {
    if (ImGui::Button(*add)) {
      in_handle_view.emplace<user>();
      return true;
    }
  } else {
    if (ImGui::InputText(*id, &in_handle_view.get<user>().get_name())) {
      in_handle_view.patch<user>();
      return true;
    }
  }
}
}  // namespace doodle::gui::render