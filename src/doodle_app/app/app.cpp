//
// Created by TD on 2022/9/29.
//

#include "app.h"
#include <doodle_app/platform/win/windows_proc.h>
#include <doodle_core/core/file_sys.h>
#include <doodle_core/thread_pool/process_pool.h>
#include <doodle_app/lib_warp/imgui_warp.h>
#include <gui/main_menu_bar.h>
#include <gui/main_status_bar.h>
#include <doodle_core/platform/win/drop_manager.h>
#include <doodle_app/app/short_cut.h>
#include <doodle_app/gui/get_input_dialog.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/gui_template/gui_process.h>
#include <doodle_core/core/init_register.h>
#include <gui/main_proc_handle.h>
#include <gui/get_input_dialog.h>

#include <doodle_app/app/program_options.h>
#include <doodle_app/lib_warp/icon_font_macro.h>
#include <boost/locale.hpp>

#include <implot.h>
#include <implot_internal.h>

// Helper functions
#include <d3d11.h>
#include <tchar.h>
// 启用窗口拖拽导入头文件
#include <shellapi.h>

#include <imgui.h>

namespace doodle {
/**
 * @brief 内部类
 *
 */
class doodle_main_app::impl {
 public:
  /// \brief 初始化 com
  [[maybe_unused]] win::ole_guard _guard{};

  win::d3d_device_ptr d3d_attr;

 public:
};

doodle_main_app::doodle_main_app(const in_gui_arg& in_arg)
    : app_command_base(in_arg),
      p_i(std::make_unique<impl>()) {
  g_reg()->ctx().emplace<gui::main_proc_handle>();
  g_reg()->ctx().emplace<gui::detail::layout_tick>();
}

void doodle_main_app::post_constructor() {
  app_command_base::post_constructor();
}

void doodle_main_app::set_title(const std::string& in_title) {
  boost::asio::post(g_io_context(), [&, in_title]() {
    auto l_str = boost::locale::conv::utf_to_utf<wchar_t>(in_title);
    SetWindowTextW(p_hwnd, l_str.c_str());
  });
}

doodle_main_app& doodle_main_app::Get() {
  return *(dynamic_cast<doodle_main_app*>(self));
}
bool doodle_main_app::valid() const {
}
void doodle_main_app::close_windows() {
  doodle::app_command_base::Get().stop_app();
  boost::asio::post([this]() {
    ::ShowWindow(p_hwnd, SW_HIDE);
    ::DestroyWindow(doodle::doodle_main_app::Get().p_hwnd);
  });
}
void doodle_main_app::show_windows() {
  ::ShowWindow(p_hwnd, SW_SHOW);
}

doodle_main_app::~doodle_main_app() {
  // Cleanup
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();
  g_reg()->ctx().at<std::shared_ptr<win::d3d_device>>().reset();

  ::RevokeDragDrop(p_hwnd);
  ::DestroyWindow(p_hwnd);
  ::UnregisterClassW(p_win_class.lpszClassName, p_win_class.hInstance);
}

bool doodle_main_app::chick_authorization() {
  if (!app_command_base::chick_authorization()) {
    auto show_str = fmt::format("授权失败\n请见授权文件放入 {} ", core_set::get_set().get_doc() / doodle_config::token_name.data());
    ::MessageBoxExW(p_hwnd, boost::locale::conv::utf_to_utf<wchar_t>(show_str).c_str(), L"错误", MB_OK, 0);
    return false;
  }
  return true;
}

}  // namespace doodle
