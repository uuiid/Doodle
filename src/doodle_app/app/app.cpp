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
#include <doodle_core/core/program_info.h>

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

doodle_main_app::doodle_main_app(const in_gui_arg& in_arg)
    : app_command_base(in_arg) {
  g_reg()->ctx().emplace<gui::main_proc_handle>();
  g_reg()->ctx().emplace<gui::detail::layout_tick>();
  auto& l_p = g_reg()->ctx().at<program_info>();
  l_p.parent_windows_attr(in_arg.in_parent);
}

doodle_main_app& doodle_main_app::Get() {
  return *(dynamic_cast<doodle_main_app*>(self));
}
bool doodle_main_app::chick_authorization() {
  return app_command_base::chick_authorization();
}

doodle_main_app::~doodle_main_app() = default;

}  // namespace doodle
