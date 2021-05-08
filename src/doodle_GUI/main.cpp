/*
 * @Author: your name
 * @Date: 2020-11-16 19:05:15
 * @LastEditTime: 2020-11-30 10:53:01
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\main.cpp
 */
#include <corelib/core/coreset.h>

#include <doodle_GUI/source/mainWidght/mainWindows.h>
#include <doodle_GUI/source/mainWidght/systemTray.h>
#include <doodle_GUI/source/SettingWidght/settingWidget.h>
#include <doodle_GUI/main.h>

#include <DoodleConfig.h>
#include <boost/locale.hpp>
#include <exception>

#include <doodle_GUI/source/Metadata/MetadataWidget.h>

wxIMPLEMENT_APP_NO_MAIN(doodle::Doodle);

namespace doodle{
Doodle::Doodle()
    : p_mainWindwos(nullptr),
      p_setting_widget(nullptr) {
  ;
};

int Doodle::OnExit() {
  if (p_mainWindwos->Close(true))
    p_mainWindwos->Destroy();
  if (p_systemTray)
    p_systemTray->Destroy();

  return wxApp::OnExit();
}

void Doodle::openMainWindow() const{
  p_mainWindwos->Show();
}

void Doodle::openSettingWindow() const{
  p_setting_widget->Show();
}
void Doodle::openMetadaWindow() const{
  p_metadata_widget->Show();
}
// bool Doodle::OnExceptionInMainLoop() {
//   this->Exception();
//   try {
//     throw;
//   } catch (const std::exception& error) {
//     auto dig    = wxMessageDialog{p_mainWindwos, ConvStr<wxString>(error.what()), ConvStr<wxString>("错误")};
//     auto result = dig.ShowModal();
//     return result == wxID_OK;
//   }
// }

bool Doodle::OnInit() {
  wxApp::OnInit();
  // wxApp::SetExitOnFrameDelete(false);
  wxLog::EnableLogging(true);

  p_mainWindwos = new mainWindows{};
  p_mainWindwos->SetIcon(wxICON(ID_DOODLE_ICON));
  this->SetTopWindow(p_mainWindwos);

  p_systemTray = new systemTray{};
  p_systemTray->SetIcon(wxICON(ID_DOODLE_ICON),
                        wxString::Format(
                            wxString{"doodle-%d.%d.%d.%d"},
                            Doodle_VERSION_MAJOR,
                            Doodle_VERSION_MINOR,
                            Doodle_VERSION_PATCH,
                            Doodle_VERSION_TWEAK));
  p_setting_widget = new SettingWidght{p_mainWindwos, wxID_ANY};
  p_mainWindwos->Show();

  p_metadata_widget = new MetadataWidget{p_mainWindwos,wxID_ANY};
  p_metadata_widget->Show();

  return true;
}

} // namespace doodle


extern "C" int WINAPI WinMain(HINSTANCE hInstance,
                              HINSTANCE hPrevInstance,
                              wxCmdLineArgType WXUNUSED(lpCmdLine),
                              int nCmdShow) try {
  //设置一下文件系统后端
  auto k_local = boost::locale::generator().generate("");
  boost::filesystem::path::imbue(k_local);
  //初始化log
  Logger::doodle_initLog();

  //初始化设置
  auto &set = doodle::coreSet::getSet();
  set.init();
  auto result = wxEntry(hInstance, hPrevInstance, NULL, nCmdShow);
  boost::log::core::get()->remove_all_sinks();

  return result;
} catch (const std::exception &err) {
  DOODLE_LOG_ERROR(err.what());
  doodle::coreSet::getSet().writeDoodleLocalSet();
  boost::log::core::get()->remove_all_sinks();
  return 1;
} catch (...) {
  doodle::coreSet::getSet().writeDoodleLocalSet();
  boost::log::core::get()->remove_all_sinks();
  return 1;
}
