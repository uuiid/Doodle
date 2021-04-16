//
// Created by teXiao on 2020/10/19.
//
#include <doodle_GUI/source/mainWidght/systemTray.h>
#include <DoodleConfig.h>

#include <loggerlib/Logger.h>
#include <corelib/core_Cpp.h>
#include <boost/format.hpp>

#include <doodle_GUI/source/mainWidght/mainWindows.h>
#include <doodle_GUI/source/toolkit/toolkit.h>

DOODLE_NAMESPACE_S

systemTray::systemTray(wxTaskBarIconType iconType)
    : wxTaskBarIcon(iconType),
      p_tool_id(wxWindow::NewControlId()),
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
  auto menu = new wxMenu();

  menu->Append(p_tool_id, _(wxString::FromUTF8("工具箱")), _(wxString::FromUTF8("打开工具箱")));
  menu->Append(p_quit_id, _(wxString::FromUTF8("退出")));
  Bind(
      wxEVT_MENU, [](wxCommandEvent& event) -> void {
        wxGetApp().Exit();
      },
      p_quit_id);
  return menu;
}

void systemTray::installMayaPlug() {
}

void systemTray::installUe4Plug(const installModel& model) {
}

void systemTray::showRigister() {
}

void systemTray::doodleQuery() {
}

void systemTray::upDoodle() {
}

DOODLE_NAMESPACE_E