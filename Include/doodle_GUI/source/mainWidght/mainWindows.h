/*
 * @Author: your name
 * @Date: 2020-09-29 17:22:20
 * @LastEditTime: 2020-10-09 16:23:08
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\mainWindows.h
 */
#pragma once

#include <doodle_GUI/doodle_global.h>

DOODLE_NAMESPACE_S

class mainWindows : public wxFrame {
  void setProgress(int value);

 public:
  explicit mainWindows();
  //~mainWindows() override;
  DOODLE_DISABLE_COPY(mainWindows);

 private:
  void doodle_init();
  void doodle_createAction();

 public:
  void openSetting();

 private:
};

class Doodle : public wxApp {
 public:
  Doodle();

  virtual bool OnInit() override;
};

DOODLE_NAMESPACE_E