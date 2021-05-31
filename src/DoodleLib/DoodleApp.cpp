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
#include <DoodleLib/mainWidght/MklinkWidget.h>
#include <DoodleLib/mainWidght/mainWindows.h>
#include <DoodleLib/mainWidght/systemTray.h>
#include <wx/cmdline.h>
#include <wx/wxprec.h>

#include <boost/algorithm/string.hpp>
#include <exception>

wxIMPLEMENT_APP_NO_MAIN(doodle::Doodle);

namespace doodle {
Doodle::Doodle()
    : p_mainWindwos(nullptr),
      p_setting_widget(nullptr) {
  ;
};

int Doodle::OnExit() {
  p_mainWindwos->Destroy();
  p_systemTray->Destroy();
  CoreSet::getSet().GetMetadataSet().clear();
  return wxApp::OnExit();
}

void Doodle::OnInitCmdLine(wxCmdLineParser& parser) {
  // parser.SetSwitchChars(ConvStr<wxString>("-"));
  wxApp::OnInitCmdLine(parser);
  parser.AddSwitch("fun");
  for (const auto& name : magic_enum::enum_names<funName>()) {
    parser.AddOption(ConvStr<wxString>(std::string{name}));
  }
}

bool Doodle::OnCmdLineParsed(wxCmdLineParser& parser) {
  wxString k_string{};
  if (parser.Found("fun")) {
    if (parser.Found(
            ConvStr<wxString>(std::string(magic_enum::enum_name(funName::mklink))),
            &k_string)) {
      //创建功能
      p_run_fun = [k_string]() {
        std::vector<std::string> str;
        boost::split(str, ConvStr<std::string>(k_string), boost::is_any_of(";"));
        if (str.size() % 2 == 0) {
          const auto k_size = str.size();
          for (auto i = 0; i < k_size; ++i) {
            MklinkWidget::mklink(str[i], str[i + 1]);
            ++i;
          }
        } else {
          DOODLE_LOG_INFO("来源和目标不匹配,无法映射");
        }
      };
    }
  }

  if(p_run_fun){
    p_run_fun = [this](){
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

      p_metadata_widget = new MetadataWidget{p_mainWindwos, wxID_ANY};
    };
  }

  return wxApp::OnCmdLineParsed(parser);
}

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
  p_run_fun();
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

  if (p_run_fun) {
    runCommand();
    return true;
  }
  return true;
}

}  // namespace doodle
