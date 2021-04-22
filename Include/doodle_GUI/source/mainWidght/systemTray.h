﻿/*
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
#include <doodle_GUI/doodle_global.h>

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
  void installMayaPlug();
  void installUe4Plug(const installModel& model);
  void doodleQuery();
  void showRigister();
  void upDoodle();

 private:
  wxWindowIDRef p_tool_id;

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
