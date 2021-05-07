﻿//
// Created by TD on 2021/5/7.
//

#pragma once
#include <doodle_GUI/doodle_global.h>
namespace doodle{
class SettingWidght;
class systemTray;
class mainWindows;
class Doodle : public wxApp {
 public:
  Doodle();

  virtual bool OnInit() override;
  virtual int OnExit() override;

  void openMainWindow();
  void openSettingWindow();

  // virtual bool OnExceptionInMainLoop() override;

 private:
  mainWindows* p_mainWindwos;
  SettingWidght* p_setting_widget;
  systemTray* p_systemTray;
};
}