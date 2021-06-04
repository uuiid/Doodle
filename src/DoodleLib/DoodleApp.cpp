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
#include <DoodleLib/Server/ServerWidget.h>
#include <DoodleLib/SettingWidght/settingWidget.h>
#include <DoodleLib/mainWidght/MklinkWidget.h>
#include <DoodleLib/mainWidght/mainWindows.h>
#include <DoodleLib/mainWidght/systemTray.h>
#include <wx/cmdline.h>
#include <wx/windowptr.h>
#include <wx/wxprec.h>

#include <boost/algorithm/string.hpp>
#include <exception>

wxIMPLEMENT_APP_NO_MAIN(doodle::Doodle);

namespace doodle {

DOODLE_STR_S(fun)
DOODLE_STR_S(server)

Doodle::Doodle()
    : wxApp(),
      p_mainWindwos(),
      p_setting_widget(),
      p_systemTray(),
      p_metadata_widget(),
      p_server_widget(),
      p_run_fun() {
  ;
};

int Doodle::OnExit() {
  if (p_mainWindwos)
    p_mainWindwos->Destroy();
  if (p_systemTray)
    p_systemTray->Destroy();

  CoreSet::getSet().clear();

  return wxApp::OnExit();
}

void Doodle::OnInitCmdLine(wxCmdLineParser& parser) {
  // parser.SetSwitchChars(ConvStr<wxString>("-"));
  wxApp::OnInitCmdLine(parser);
  parser.AddSwitch(staticValue::funObj());
  for (const auto& name : magic_enum::enum_names<funName>()) {
    parser.AddOption(ConvStr<wxString>(std::string{name}));
  }
  parser.AddSwitch(staticValue::serverObj());
}

bool Doodle::OnCmdLineParsed(wxCmdLineParser& parser) {
  wxString k_string{};
  if (parser.Found(staticValue::funObj())) {
    if (parser.Found(
            ConvStr<wxString>(std::string(magic_enum::enum_name(funName::mklink))),
            &k_string)) {
      //创建功能
      p_run_fun = [k_string, this]() { funMklink(k_string); };
      return wxApp::OnCmdLineParsed(parser);
    }
  }

  if (parser.Found(staticValue::serverObj())) {
    p_run_fun = [this]() { serverInit(); };
    return wxApp::OnCmdLineParsed(parser);
  }

  if (!p_run_fun) {
    p_run_fun = [this]() { guiInit(); };
  }

  return wxApp::OnCmdLineParsed(parser);
}
void Doodle::funMklink(const wxString& k_string) {
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
  Exit();
}
void Doodle::guiInit() {
  CoreSet::getSet().guiInit();
  const wxIcon& k_icon = wxICON(ID_DOODLE_ICON);

  p_mainWindwos = new mainWindows{};
  p_mainWindwos->SetIcon(k_icon);
  SetTopWindow(p_mainWindwos);

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
  p_metadata_widget->Show();
}

void Doodle::openMainWindow() {
  p_mainWindwos->Show();
}

void Doodle::openSettingWindow() {
  p_setting_widget = new SettingWidght{p_mainWindwos, wxID_ANY};
  p_setting_widget->Show();
}
void Doodle::openMetadaWindow() {
  p_metadata_widget = new MetadataWidget{p_mainWindwos, wxID_ANY};
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
  if (!wxApp::OnInit())
    return false;

  // wxApp::SetExitOnFrameDelete(false);
  wxLog::EnableLogging(true);

  for (int i = 0; i < argc; ++i) {
    DOODLE_LOG_INFO("arg " << i << ": " << argv[i]);
  }

  if (p_run_fun) {
    p_run_fun();
    return true;
  }
  return true;
}
void Doodle::serverInit() {
  p_server_widget = new ServerWidget{};
  p_server_widget->Show();
}

}  // namespace doodle
