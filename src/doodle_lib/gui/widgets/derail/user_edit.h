//
// Created by td_main on 2023/6/9.
//

#pragma once
namespace doodle::gui::render {

// class user_edit_t {};
using entt::literals::operator""_hs;
using user_edit_t = entt::tag<"user_edit_t"_hs>;

bool user_edit(const entt::handle& in_handle_view);

}  // namespace doodle::gui::render
