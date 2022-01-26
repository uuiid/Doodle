//
// Created by TD on 2022/1/18.
//

#pragma once
#include <doodle_lib/core/app_base.h>

namespace doodle {
class DOODLELIB_API app : public app_command_base {
 protected:
  win::wnd_class p_win_class;

  std::shared_ptr<win::d3d_device> d3d_deve;
  bool p_show_err;

 public:
  explicit app(const win::wnd_instance& in_instance);
  explicit app();
  ~app() override;
  win::wnd_handle p_hwnd;
  ::ID3D11Device* d3dDevice;
  ::ID3D11DeviceContext* d3dDeviceContext;

  static app& Get();
  bool valid() const override;

  void loop_one() override;

  virtual void hide_windows();
  virtual void show_windows();

 protected:
  virtual void load_windows();
  void load_back_end() override;

  void set_imgui_dock_space(const FSys::path& in_path) const;
};
}  // namespace doodle
