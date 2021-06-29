//
// Created by TD on 2021/6/29.
//

#include "nana_warp.h"
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
}  // namespace doodle
