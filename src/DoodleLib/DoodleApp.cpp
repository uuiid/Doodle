/*
 * @Author: your name
 * @Date: 2020-11-16 19:05:15
 * @LastEditTime: 2020-11-30 10:53:01
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\main.cpp
 */
#include <core/coreset.h>

#include <mainWidght/mainWindows.h>
#include <mainWidght/systemTray.h>
#include <SettingWidght/settingWidget.h>
#include <DoodleApp.h>

#include <DoodleConfig.h>
#include <exception>

#include <Metadata/MetadataWidget.h>
//#include <DoodleLib/win_exe.rc>

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
  const wxIcon& k_icon = wxICON(ID_DOODLE_ICON);
  p_mainWindwos->SetIcon(k_icon);
  this->SetTopWindow(p_mainWindwos);

  p_systemTray = new systemTray{};
  p_systemTray->SetIcon(k_icon,
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
