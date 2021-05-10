//
// Created by TD on 2021/5/7.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
namespace doodle {
class SettingWidght;
class systemTray;
class mainWindows;
class MetadataWidget;

class DOODLELIB_API Doodle : public wxApp {
 public:
  Doodle();

  virtual bool OnInit() override;
  virtual int OnExit() override;
  void OnInitCmdLine(wxCmdLineParser& parser) override;
  // bool OnCmdLineParsed(wxCmdLineParser& parser) override;

  void openMainWindow() const;
  void openSettingWindow() const;
  void openMetadaWindow() const;
  // virtual bool OnExceptionInMainLoop() override;

 private:
  void runCommand();
  mainWindows* p_mainWindwos;
  SettingWidght* p_setting_widget;
  systemTray* p_systemTray;
  MetadataWidget* p_metadata_widget;
};
}  // namespace doodle
