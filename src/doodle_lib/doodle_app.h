//
// Created by TD on 2021/9/14.
//

#pragma once

#include <doodle_lib/Exception/exception.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <Windows.h>

#include <boost/signals2.hpp>
namespace doodle {
using win_handle = HWND;
using win_class  = WNDCLASSEX;

class long_time_tasks_widget;
class DOODLELIB_API doodle_app : public details::no_copy {
  win_handle p_hwnd;
  win_class p_win_class;
  doodle_app();
  static doodle_app* self;

 public:
  std::atomic_bool p_done;

  using connection = boost::signals2::connection;

  static std::unique_ptr<doodle_app> make_this();
  static doodle_app* Get();
  boost::signals2::signal<void()> main_loop;

  std::shared_ptr<long_time_tasks_widget> long_task_widgets;
  widget_register_ptr wregister;

  inline widget_register_ptr get_register(){return wregister;};
  inline const widget_register_ptr get_register() const {return wregister;};

  std::int32_t run();
  ~doodle_app();
};
}  // namespace doodle
