//
// Created by TD on 2021/9/14.
//

#pragma once

#include <Windows.h>
#include <doodle_lib/exception/exception.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <imgui.h>

#include <boost/signals2.hpp>
struct ID3D11Device;

namespace doodle {
using win_handle = HWND;
using win_class  = WNDCLASSEX;

class long_time_tasks_widget;
class DOODLELIB_API doodle_app : public details::no_copy {
  win_handle p_hwnd;
  win_class p_win_class;
  static doodle_app* self;

  virtual base_widget_ptr get_main_windows() const;
  std::wstring p_title;

  void set_imgui_dock_space(const FSys::path& in_path) const;
  entt::observer k_metadata_obs;

  bool p_show_err;
  base_widget_ptr p_main_win;

 public:
  doodle_app();
  std::atomic_bool p_done;

  using connection = boost::signals2::connection;
  virtual void post_constructor();

  static doodle_app* Get();
  boost::signals2::signal<void()> main_loop;

  std::shared_ptr<long_time_tasks_widget> long_task_widgets;
  widget_register_ptr wregister;
  ID3D11Device* p_pd3dDevice;

  inline widget_register_ptr get_register() { return wregister; };
  inline const widget_register_ptr get_register() const { return wregister; };

  bool valid() const;

  virtual std::int32_t run();
  ~doodle_app();

  virtual base_widget_ptr loop_begin();
  virtual void loop_one();

  virtual void hide_windows();

 private:
  void metadata_load();
  void metadata_save() const;
  void metadata_delete() const;

  void metadata_loop_one();

 protected:
};
}  // namespace doodle
