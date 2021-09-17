//
// Created by TD on 2021/9/14.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Exception/Exception.h>
#include <Windows.h>

#include <boost/signals2.hpp>
namespace doodle {
using win_handle = HWND;
using win_class  = WNDCLASSEX;
class DOODLELIB_API doodle_app : public details::no_copy {
  win_handle p_hwnd;
  win_class p_win_class;
  std::set<std::shared_ptr<std::function<bool()>>> p_fun_list;
  doodle_app();
  static doodle_app* self;

 public:
  std::atomic_bool p_done;

  using connection = boost::signals2::connection;

  static std::unique_ptr<doodle_app> make_this();
  static doodle_app* Get();
  boost::signals2::signal<void()> main_loop;
  std::int32_t run();
  ~doodle_app();
};
}  // namespace doodle
