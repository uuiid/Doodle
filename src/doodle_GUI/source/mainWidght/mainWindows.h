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
class SettingWidght;
class systemTray;

class mainWindows : public wxFrame {
  wxWindowIDRef p_exmaya_id;
  wxWindowIDRef p_create_image_id;
  wxWindowIDRef p_create_dir_image_id;
  wxWindowIDRef p_create_video_id;
  wxWindowIDRef p_create_ue4File_id;

  void setProgress(int value);

 public:
  explicit mainWindows();

  DOODLE_DISABLE_COPY(mainWindows);

 private:
  void exportMayaFile(const std::vector<FSys::path>& paths);
  void createVideoFile(const std::vector<FSys::path>& paths);
  void createVideoFileFormDir(const std::vector<FSys::path>& paths);
  void connectVideo(const std::vector<FSys::path>& paths);
  void createUe4Project(const std::vector<FSys::path>& paths);
};

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

DOODLE_NAMESPACE_E