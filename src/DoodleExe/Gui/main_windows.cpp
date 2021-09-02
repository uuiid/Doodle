//
// Created by TD on 2021/6/24.
//

#include "main_windows.h"

#include <DoodleExe/Gui/Metadata/project_widget.h>
#include <DoodleExe/Gui/factory/tool_box_menu_factory.h>
#include <DoodleExe/Gui/setting_windows.h>
#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/core/DoodleLib.h>
#include <DoodleLib/rpc/RpcFileSystemClient.h>
#include <DoodleLib/toolkit/toolkit.h>

#include <nana/gui/filebox.hpp>

namespace doodle {

main_windows::main_windows()
    : nana::form(nana::API::make_center(1200, 1000)),
      p_layout(*this),
      p_menubar(*this),
      p_menu(),
      p_project_listbox(std::make_shared<project_widget>(*this)),
      p_ass_tree_box(std::make_shared<assets_widget>(*this)),
      p_attr_listbox(std::make_shared<assets_attr_widget>(*this)),
      p_menu_factory(std::make_shared<tool_box_menu_factory>(*this)),
      p_setting_windows() {
  p_layout.div(
      R"(
<
  vertical 

  <weight=23 menubar> 
  <weight=20% project_listbox> |
    < 
      <weight=30% ass_tree_box> | <attr_listbox > 
    > 
>
)");
  create_menubar();

  p_layout.field("menubar") << p_menubar;
  p_layout.field("project_listbox") << p_project_listbox->get_widget();
  p_layout.field("ass_tree_box") << p_ass_tree_box->get_widget();
  p_layout.field("attr_listbox") << p_attr_listbox->get_widget();
  p_layout.collocate();

  p_project_listbox->sig_selected.connect([this](const MetadataPtr& in_, bool is_selected) {
    if (is_selected)
      p_ass_tree_box->set_ass(std::dynamic_pointer_cast<Project>(in_));
    else {
      p_ass_tree_box->clear();
      p_attr_listbox->clear();
    }
  });
  p_ass_tree_box->sig_selected.connect([this](const MetadataPtr& in_ptr, bool is_) {
    if (is_)
      this->p_attr_listbox->set_ass(in_ptr);
    else
      this->p_attr_listbox->clear();
  });
}

void main_windows::create_menubar() {
  auto& k_file_menu = p_menubar.push_back("文件");
  k_file_menu.append("设置", [this](nana::menu::item_proxy&) {
    p_setting_windows = std::make_shared<setting_windows>(*this);
    p_setting_windows->show();
  });
  k_file_menu.append("退出", [](nana::menu::item_proxy&) { nana::API::exit_all(); });
  auto& k_tool_menu = p_menubar.push_back("工具箱");

  (*p_menu_factory)(k_tool_menu);
  k_tool_menu.append_splitter();

  k_tool_menu.append("安装maya插件", [this](nana::menu::item_proxy&) {
    toolkit::installMayaPath();
    nana::msgbox mes{*this, "完成maya插件安装"};
    mes();
  });
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
      nana::msgbox mes{*this, "错误"};
      mes << error.what();
      mes();
    }
  });
  k_tool_menu.append("安装ue项目插件", [this](nana::menu::item_proxy&) {
    auto k_file_dig = nana::filebox{*this, true};
    k_file_dig.add_filter("ue project files", "*.uproject;");
    auto k_f = k_file_dig();
    if (!k_f.empty()) {
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
  // k_tool_menu.append("更新", [](nana::menu::item_proxy&) {
  //   try {
  //     auto k_rpc  = DoodleLib::Get().getRpcFileSystemClient();
  //     auto k_name = fmt::format(
  //         "Doodle-{}.{}.{}-win64.msi",
  //         Doodle_VERSION_MAJOR,
  //         Doodle_VERSION_MINOR,
  //         Doodle_VERSION_PATCH);
  //     auto k_l = CoreSet::getSet().getCacheRoot("update") / k_name;
  //     auto k_s = FSys::path{"html"} /
  //                "file" /
  //                k_name;

  //     k_rpc->Download(k_l, k_s);
      
  //   } catch (const DoodleError& e) {
  //     DOODLE_LOG_ERROR(e.what());
  //   }
  // });
}
}  // namespace doodle
