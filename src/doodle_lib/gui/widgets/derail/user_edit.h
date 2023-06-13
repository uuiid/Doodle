//
// Created by td_main on 2023/6/9.
//

#pragma once
#include "doodle_core/core/core_help_impl.h"

#include <doodle_app/gui/base/ref_base.h>

#include "entt/entity/fwd.hpp"
#include <string>
#include <tuple>
#include <utility>
#include <vector>
namespace doodle::gui::render {

class select_all_user_t {
  std::string current_user{};

  void refresh(const registry_ptr& in_reg_ptr);

 public:
  select_all_user_t() = default;
  std::vector<std::pair<std::string, entt::handle>> user_list{};
  gui_cache_name_id user_id{"用户"s};

  void set_current_user(const std::string& in_current_user);
  void set_current_user(const entt::handle& in_current_user);
  std::tuple<bool, entt::handle> render(const registry_ptr& in_reg_ptr);
};

class user_edit_t {
  gui_cache_name_id id{"用户"s};
  gui_cache_name_id add{"添加用户"s};

 public:
  bool render(const entt::handle& in_handle_view);
};

}  // namespace doodle::gui::render
