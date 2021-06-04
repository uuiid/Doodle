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
class ServerWidget;

enum class funName {
  mklink,
};

DOODLE_STR_S_DECLARE(fun)
DOODLE_STR_S_DECLARE(server)

class wxWindowDeleter final {
 public:
  void operator()(wxWindow* win) {
    if (win)
      win->Destroy();
  }
};

class DOODLELIB_API Doodle : public wxApp {
 public:
  Doodle();

  virtual bool OnInit() override;
  virtual int OnExit() override;
  void OnInitCmdLine(wxCmdLineParser& parser) override;
  bool OnCmdLineParsed(wxCmdLineParser& parser) override;

  void openMainWindow() ;
  void openSettingWindow() ;
  void openMetadaWindow();
  // virtual bool OnExceptionInMainLoop() override;

 private:
  mainWindows* p_mainWindwos;
  SettingWidght* p_setting_widget;
  systemTray* p_systemTray;
  MetadataWidget* p_metadata_widget;
  ServerWidget* p_server_widget;
  std::function<void()> p_run_fun;

  void guiInit();
  void funMklink(const wxString& k_string);
  void serverInit();
};

}  // namespace doodle
