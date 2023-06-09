//
// Created by td_main on 2023/6/9.
//

#pragma once
#include "doodle_core/core/core_help_impl.h"

#include <doodle_app/gui/base/ref_base.h>

#include "entt/entity/fwd.hpp"
#include <tuple>
#include <utility>
#include <vector>
namespace doodle::gui::render {

// class user_edit_t {};
using entt::literals::operator""_hs;
using user_edit_t = entt::tag<"user_edit_t"_hs>;
class select_all_user_t {
 public:
  select_all_user_t() = default;
  std::vector<std::pair<std::string, entt::handle>> user_list{};
  gui_cache_name_id user_id{"用户"s};

  void refresh(const registry_ptr& in_reg_ptr);
};

bool user_edit(const entt::handle& in_handle_view);

std::tuple<bool, entt::handle> select_all_user(const std::string& in_current_user, const registry_ptr& in_reg_ptr);
std::tuple<bool, entt::handle> select_all_user(const entt::handle& in_current_user, const registry_ptr& in_reg_ptr);

}  // namespace doodle::gui::render
