//
// Created by teXiao on 2020/10/19.
//
#include <DoodleApp.h>
#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/FileSys/FileSystem.h>
#include <DoodleLib/FileWarp/ImageSequence.h>
#include <DoodleLib/FileWarp/MayaFile.h>
#include <DoodleLib/FileWarp/Ue4Project.h>
#include <DoodleLib/FileWarp/VideoSequence.h>
#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/Metadata/Shot.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/mainWidght/systemTray.h>
#include <toolkit/toolkit.h>

#include <boost/format.hpp>

DOODLE_NAMESPACE_S

systemTray::systemTray(wxTaskBarIconType iconType)
    : wxTaskBarIcon(iconType),
      p_tool_id(wxWindow::NewControlId()),
      p_Meta_id(wxWindow::NewControlId()),
      p_installMayaPlug_id(wxWindow::NewControlId()),
      p_installUEPlug_id(wxWindow::NewControlId()),
      p_installUEProjectPlug_id(wxWindow::NewControlId()),
      p_deleteUECache_id(wxWindow::NewControlId()),
      p_modifyUECache_id(wxWindow::NewControlId()),
      p_setting_id(wxWindow::NewControlId()),
      p_updata_id(wxWindow::NewControlId()),
      p_quit_id(wxWindow::NewControlId()) {
}

wxMenu* systemTray::CreatePopupMenu() {
  auto menu = new wxMenu{};

  menu->Append(p_Meta_id, ConvStr<wxString>("主窗口"));
  menu->Append(p_tool_id, ConvStr<wxString>("工具箱"), ConvStr<wxString>("打开工具箱"));
  menu->Append(p_setting_id, ConvStr<wxString>("打开设置"));
  menu->AppendSeparator();

  auto menu_install = new wxMenu{};
  menu->AppendSubMenu(menu_install, ConvStr<wxString>("插件安装"));
  menu_install->Append(p_installMayaPlug_id, ConvStr<wxString>("安装maya插件"));
  menu_install->Append(p_installUEPlug_id, ConvStr<wxString>("安装ue插件"));
  menu_install->Append(p_installUEProjectPlug_id, ConvStr<wxString>("安装ue项目插件"));

  menu->AppendSeparator();
  menu->Append(p_deleteUECache_id, ConvStr<wxString>("删除ue4缓存"));
  menu->Append(p_modifyUECache_id, ConvStr<wxString>("修改ue4缓存位置"));

  menu->AppendSeparator();
  menu->Append(p_updata_id, ConvStr<wxString>("更新"));

  menu->Append(p_quit_id, ConvStr<wxString>("退出"));

  //打开元数据窗口
  menu->Bind(
      wxEVT_MENU, [](wxCommandEvent& event) {
        wxGetApp().openMetadaWindow();
      },
      p_Meta_id);
  //打开工具箱
  menu->Bind(
      wxEVT_MENU, [](wxCommandEvent& event) {
        wxGetApp().openMainWindow();
      },
      p_tool_id);
  //打开设置
  menu->Bind(
      wxEVT_MENU, [](wxCommandEvent& event) {
        wxGetApp().openSettingWindow();
      },
      p_setting_id);
  //安装ue插件到总体
  menu_install->Bind(
      wxEVT_MENU, [](wxCommandEvent& event) {
        auto& k_ue_4_setting = coreSet::getSet().gettUe4Setting();
        if (k_ue_4_setting.hasPath())
          toolkit::installUePath(k_ue_4_setting.Path() / "Engine");
        else
          wxMessageDialog{wxGetApp().GetTopWindow(),
                          ConvStr<wxString>("在设置中找不到ue位置")}
              .ShowModal();
      },
      p_installUEPlug_id);
  //安装ue插件到项目
  menu_install->Bind(
      wxEVT_MENU, [](wxCommandEvent& event) {
        auto file_dig = wxFileDialog{
            wxGetApp().GetTopWindow(),
            ConvStr<wxString>("ue项目选择"),
            wxEmptyString,
            wxEmptyString,
            ConvStr<wxString>("files (*.uproject)|*.uproject")};
        auto result = file_dig.ShowModal();
        if (result == wxID_OK) {
          FSys::path path{ConvStr<std::string>(file_dig.GetPath())};
          toolkit::installUePath(path.parent_path());
        }
      },
      p_installUEProjectPlug_id);
  //安装maya插件
  menu_install->Bind(
      wxEVT_MENU,
      [](wxCommandEvent& event) {
        toolkit::installMayaPath();
      },
      p_installMayaPlug_id);
  //删除ue缓存
  menu->Bind(
      wxEVT_MENU, [](wxCommandEvent& event) {
        try {
          toolkit::deleteUeCache();
        } catch (const std::exception& error) {
          wxMessageDialog{wxGetApp().GetTopWindow(), ConvStr<wxString>(error.what())}.ShowModal();
          return;
        }
        wxMessageDialog{wxGetApp().GetTopWindow(),
                        ConvStr<wxString>("完成删除")}
            .ShowModal();
      },
      p_deleteUECache_id);
  //修改ue缓存
  menu->Bind(
      wxEVT_MENU, [](wxCommandEvent& event) {
        try {
          toolkit::modifyUeCachePath();
        } catch (const std::exception& error) {
          wxMessageDialog{wxGetApp().GetTopWindow(), ConvStr<wxString>(error.what())}.ShowModal();
          return;
        }
        wxMessageDialog{wxGetApp().GetTopWindow(), ConvStr<wxString>("完成修改")}.ShowModal();
      },
      p_modifyUECache_id);

  //更新
  menu->Bind(
      wxEVT_MENU, [](wxCommandEvent& event) {
        wxMessageDialog{wxGetApp().GetTopWindow(), ConvStr<wxString>("暂时不支持动态更新")}.ShowModal();
      },
      p_updata_id);
  //退出
  menu->Bind(
      wxEVT_MENU, [](wxCommandEvent& event) -> void {
        wxGetApp().Exit();
      },
      p_quit_id);

  return menu;
}

DOODLE_NAMESPACE_E
