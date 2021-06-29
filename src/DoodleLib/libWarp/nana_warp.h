//
// Created by TD on 2021/6/29.
//

#pragma once
#include <functional>
#include <nana/gui/widgets/menu.hpp>

namespace doodle{

class menu_assist
{
  std::function<void(const nana::arg_mouse &arg)> _fun;

 public:
  explicit menu_assist(decltype(_fun) &in_fun) : _fun(in_fun){};
  void operator()(const nana::arg_mouse &arg);
};
}
