//
// Created by TD on 2021/6/24.
//

#include "main_windows.h"

#include <DoodleLib/Gui/setting_windows.h>
#include <core/CoreSet.h>
#include <toolkit/toolkit.h>

#include <nana/gui/filebox.hpp>

namespace doodle {

main_windows::main_windows()
    : nana::form(nana::API::make_center(500, 600)),
      p_layout(*this),
      p_menubar(*this),
      p_menu(),
      p_setting_windows() {
  p_layout.div(
      R"(<vertical <weight=23 menubar> >)");
  create_menubar();




  p_layout.field("menubar") << p_menubar;
  p_layout.collocate();
}
void main_windows::create_menubar() {
  auto& k_file_menu = p_menubar.push_back("文件");
  k_file_menu.append("设置", [this](nana::menu::item_proxy&) {
    p_setting_windows = std::make_shared<setting_windows>(*this);
    p_setting_windows->show();
  });
  k_file_menu.append("退出", [](nana::menu::item_proxy&) { nana::API::exit_all(); });
  auto& k_tool_menu = p_menubar.push_back("工具箱");

  k_tool_menu.append("安装maya插件", [this](nana::menu::item_proxy&) {
    toolkit::installMayaPath();
    nana::msgbox mes{*this, "完成maya插件安装"};
    mes();
    ; });
  k_tool_menu.append("安装ue插件", [this](nana::menu::item_proxy&) {
    try {
      auto& k_ue_setting = CoreSet::getSet().gettUe4Setting();
      if (k_ue_setting.hasPath())
        toolkit::installUePath(k_ue_setting.Path() / "Engine");
      else {
        nana::msgbox mes{*this, "在设置中找不到ue位置"};
        mes();
      }
    } catch (const std::exception& error) {
      nana::msgbox mes{*this, error.what()};
      mes();
    }
  });
  k_tool_menu.append("安装ue项目插件", [this](nana::menu::item_proxy&) {
    auto k_file_dig = nana::filebox{*this, true};
    k_file_dig.add_filter("ue project files","*.uproject;");
    auto k_f = k_file_dig();
    if(!k_f.empty()){
      toolkit::installUePath(k_f.at(0).parent_path());
    }
  });
  k_tool_menu.append("删除ue4缓存", [this](nana::menu::item_proxy&) {
    try {
      toolkit::deleteUeCache();
    } catch (const std::exception& error) {
      nana::msgbox mes{*this, error.what()};
      mes();
    }
  });
  k_tool_menu.append("修改ue4缓存位置", [this](nana::menu::item_proxy&) {
    try {
      toolkit::modifyUeCachePath();
    } catch (const std::exception& error) {
      nana::msgbox mes{*this, error.what()};
      mes();
    }
  });
}
}  // namespace doodle
