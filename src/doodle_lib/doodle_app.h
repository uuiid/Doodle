//
// Created by TD on 2021/9/14.
//

#pragma once

#include <doodle_lib/exception/exception.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/signals2.hpp>
struct ID3D11Device;

namespace doodle {

class long_time_tasks_widget;
class DOODLELIB_API doodle_app : public details::no_copy {
  class impl;

  std::unique_ptr<impl> p_impl;
  static doodle_app* self;

  std::wstring p_title;

  void set_imgui_dock_space(const FSys::path& in_path) const;

  bool p_show_err;
  base_widget_ptr p_main_win;

 public:
  doodle_app();
  ~doodle_app();

  std::atomic_bool p_done;

  static doodle_app* Get();

  ID3D11Device* p_pd3dDevice;

  bool valid() const;

  virtual std::int32_t run();

  virtual base_widget_ptr loop_begin();
  virtual void loop_one();

  virtual void hide_windows();

 private:
 protected:
  virtual void load_windows();
  static void load_setting_windows();
};

}  // namespace doodle
