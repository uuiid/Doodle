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
  static doodle_app* self;

  virtual base_widget_ptr get_main_windows() const; 

 public:
  doodle_app();
  std::atomic_bool p_done;

  using connection = boost::signals2::connection;
  virtual void post_constructor();

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
