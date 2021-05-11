/*
 * @Author: your name
 * @Date: 2020-11-16 19:05:15
 * @LastEditTime: 2020-11-30 10:53:01
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\main.cpp
 */
#include <DoodleConfig.h>
#include <DoodleLib/DoodleApp.h>
#include <DoodleLib/Metadata/MetadataWidget.h>
#include <DoodleLib/SettingWidght/settingWidget.h>
#include <DoodleLib/mainWidght/mainWindows.h>
#include <DoodleLib/mainWidght/systemTray.h>
#include <DoodleLib/mainWidght/MklinkWidget.h>

#include <exception>
#include <wx/cmdline.h>
#include <wx/wxprec.h>
#include <boost/algorithm/string.hpp>

wxIMPLEMENT_APP_NO_MAIN(doodle::Doodle);

namespace doodle {
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

void Doodle::OnInitCmdLine(wxCmdLineParser& parser) {
  // parser.SetSwitchChars(ConvStr<wxString>("-"));
  wxApp::OnInitCmdLine(parser);
  parser.AddOption(ConvStr<wxString>("fun"));
}

// bool Doodle::OnCmdLineParsed(wxCmdLineParser& parser) {
//   auto fun = parser.Found(ConvStr<wxString>("fun"));
//   return true;
// }

void Doodle::openMainWindow() const {
  p_mainWindwos->Show();
}

void Doodle::openSettingWindow() const {
  p_setting_widget->Show();
}
void Doodle::openMetadaWindow() const {
  p_metadata_widget->Show();
}

void Doodle::runCommand() {
  auto cmd = ConvStr<std::string>(argv[1]);
  std::vector<std::string> str;
  std::vector<std::string> str2;
  boost::split(str, cmd, boost::is_any_of("="));
  boost::split(str2, str[1], boost::is_any_of(";"));
  MklinkWidget::mklink(str2[1], str2[2]);
  Exit();
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
  if (!wxApp::OnInit())
    return false;

  // wxApp::SetExitOnFrameDelete(false);
  wxLog::EnableLogging(true);

  for (int i = 0; i < argc; ++i) {
    DOODLE_LOG_INFO("arg " << i << ": " << argv[i]);
  }

  if (argc >= 2) {
    runCommand();
    return true;
  }

  p_mainWindwos        = new mainWindows{};
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

  p_metadata_widget = new MetadataWidget{p_mainWindwos, wxID_ANY};
  // p_metadata_widget->Show();

  return true;
}

}  // namespace doodle
