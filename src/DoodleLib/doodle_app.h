//
// Created by TD on 2021/9/14.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <windef.h>
#include <WinUser.h>



namespace doodle {
using win_handle = HWND;
using win_class = WNDCLASSEX;
class doodle_app {
  win_handle p_hwnd;
  win_class p_win_class;
 public:
  doodle_app();
  std::int32_t run();
  ~doodle_app();
};
}  // namespace doodle
