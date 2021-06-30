//
// Created by TD on 2021/6/29.
//

#include "nana_warp.h"
#include <date/date.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
namespace doodle {

void menu_assist::operator()(const nana::arg_mouse& arg) {
  //  decltype(arg.window_handle) owner_;
  //  decltype(arg.pos) pos_;

  switch (arg.evt_code) {
    case nana::event_code::click:
    case nana::event_code::mouse_down:
    case nana::event_code::mouse_up:
      //      owner_ = arg.window_handle;
      //      pos_   = arg.pos;
      if (nana::mouse::right_button == arg.button)
        _fun(arg);
      break;
    default:
      return;
  }
}
//nana::listbox::oresolver& operator<<(nana::listbox::oresolver& oor, const std::chrono::time_point<std::chrono::system_clock>& in_time) {
//  oor << date::format("", in_time);
//  return oor;
//}
//nana::listbox::iresolver& operator>>(nana::listbox::iresolver& oor, std::chrono::time_point<std::chrono::system_clock>& in_time) {
//  return oor;
//}
}  // namespace doodle
