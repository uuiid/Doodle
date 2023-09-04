//
// Created by TD on 2022/9/29.
//

#include "menu_bar.h"

#include <doodle_core/doodle_core_fwd.h>

#include "doodle_app/gui/base/base_window.h"
#include <doodle_app/app/app_command.h>
#include <doodle_app/gui/show_message.h>

#include <doodle_lib/render_farm/proxy_server.h>
#include <doodle_lib/render_farm/working_machine.h>
#include <doodle_lib/toolkit/toolkit.h>

#include <fmt/core.h>
#include <winreg/WinReg.hpp>
namespace doodle::gui {
menu_bar::menu_bar() {
  connection_  = app_base::Get().on_stop.connect([this]() {
    auto &&l_lib = doodle_lib::Get();
    if (l_lib.ctx().contains<doodle::render_farm::working_machine_ptr>()) {
      l_lib.ctx().get<doodle::render_farm::working_machine_ptr>()->stop();
      l_lib.ctx().erase<doodle::render_farm::working_machine_ptr>();
    }
  });
  auto &&l_lib = doodle_lib::Get();

  run_client   = false;
}

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
}

}  // namespace doodle::gui
