/*
 * @Author: your name
 * @Date: 2020-10-19 19:24:47
 * @LastEditTime: 2020-11-27 11:27:23
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\systemTray.h
 */
//
// Created by teXiao on 2020/10/19.
//
#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <wx/taskbar.h>

DOODLE_NAMESPACE_S

class systemTray : public wxTaskBarIcon {
 public:
  explicit systemTray(wxTaskBarIconType iconType = wxTBI_DEFAULT_TYPE);
  enum class installModel { peject,
                            exeFile };

 protected:
  virtual wxMenu* CreatePopupMenu() override;

 private:
  wxWindowIDRef p_tool_id;
  wxWindowIDRef p_Meta_id;

  wxWindowIDRef p_installMayaPlug_id;
  wxWindowIDRef p_installUEPlug_id;
  wxWindowIDRef p_installUEProjectPlug_id;

  wxWindowIDRef p_deleteUECache_id;
  wxWindowIDRef p_modifyUECache_id;

  wxWindowIDRef p_setting_id;
  wxWindowIDRef p_updata_id;
  wxWindowIDRef p_quit_id;
};
DOODLE_NAMESPACE_E
