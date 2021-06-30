//
// Created by TD on 2021/6/29.
//

#pragma once
#include <chrono>
#include <functional>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/menu.hpp>
namespace doodle {

class menu_assist {
  std::function<void(const nana::arg_mouse& arg)> _fun;

 public:
  explicit menu_assist(decltype(_fun)& in_fun) : _fun(in_fun){};
  void operator()(const nana::arg_mouse& arg);
};

//nana::listbox::oresolver& operator<<(nana::listbox::oresolver& oor, const std::chrono::time_point<std::chrono::system_clock>& in_time);
//nana::listbox::iresolver& operator>>(nana::listbox::iresolver& oor, std::chrono::time_point<std::chrono::system_clock>& in_time);
}  // namespace doodle
