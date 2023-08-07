//
// Created by TD on 2022/9/29.
//

#include "menu_bar.h"

#include <doodle_core/doodle_core_fwd.h>

#include "doodle_app/gui/base/base_window.h"
#include <doodle_app/app/app_command.h>
#include <doodle_app/gui/show_message.h>

#include <doodle_lib/render_farm/working_machine.h>
#include <doodle_lib/toolkit/toolkit.h>

#include <fmt/core.h>
#include <winreg/WinReg.hpp>
namespace doodle::gui {

void menu_bar::message(const std::string &in_m) {
  g_windows_manage().create_windows_arg(
      windows_init_arg{}.create<show_message>(in_m).set_title("显示消息").set_render_type<dear::Popup>()
  );
}

void menu_bar::menu_tool() {
  if (dear::MenuItem("安装maya插件")) {
    std::string l_message = "安装maya插件";
    try {
      toolkit::installMayaPath();
      l_message += "成功";
    } catch (const FSys::filesystem_error &error) {
      l_message = fmt::format("{}失败{} ", l_message, boost::diagnostic_information(error));
    }
    menu_bar::message(l_message);
  }

  if (dear::MenuItem("安装ue插件")) {
    std::string l_message = "安装ue插件{}";
    try {
      toolkit::installUePath(core_set::get_set().ue4_path / "Engine");
      l_message += "成功";
    } catch (const FSys::filesystem_error &error) {
      l_message = fmt::format("{}失败:{} ", l_message, boost::diagnostic_information(error));
    }
    menu_bar::message(l_message);
  }

  if (dear::MenuItem("删除ue缓存")) {
    std::string l_message = "删除ue缓存{}";
    try {
      toolkit::deleteUeCache();
      l_message += "成功";
    } catch (const doodle::doodle_error &error) {
      l_message = fmt::format("{}失败\n{} ", l_message, boost::diagnostic_information(error));
    }
    menu_bar::message(l_message);
  }

  if (dear::MenuItem("修改ue缓存位置")) {
    std::string l_message = "修改ue缓存位置{}";
    try {
      toolkit::modifyUeCachePath();
      l_message += "成功";
    } catch (const winreg::RegException &error) {
      l_message = fmt::format("失败(请使用管理员启动)\n{} ", boost::diagnostic_information(error));
    }
    menu_bar::message(l_message);
  }

  if (dear::MenuItem("安装houdini 19.0插件")) {
    std::string l_message = "安装houdini插件{}";
    try {
      toolkit::install_houdini_plug();
      l_message += "成功";
    } catch (const FSys::filesystem_error &error) {
      l_message = fmt::format("失败{} ", boost::diagnostic_information(error));
    }
    menu_bar::message(l_message);
  }

  if (dear::MenuItem("启动渲染客户端", &run_client)) {
    menu_start_render_client();
  }
}
void menu_bar::menu_start_render_client() {
  if (run_client) {
    doodle_lib::Get().ctx().emplace<doodle::render_farm::working_machine>(g_io_context(), "127.0.0.1", 50021).run();
  } else {
    doodle_lib::Get().ctx().emplace<doodle::render_farm::working_machine>(g_io_context(), "127.0.0.1", 50021).stop();
  }
}

}  // namespace doodle::gui
