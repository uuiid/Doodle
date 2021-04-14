#include <doodle_GUI/source/mainWidght/mainWindows.h>
//logger是boost库使用者，放到qt上面能好点
#include <loggerlib/Logger.h>

#include <doodle_GUI/source/mainWidght/DragPushBUtton.h>
#include <doodle_GUI/source/mainWidght/systemTray.h>
#include <doodle_GUI/source/toolkit/MessageAndProgress.h>
#include <doodle_GUI/source/SettingWidght/SettingWidget.h>
#include <doodle_GUI/source/Metadata/View/ShotListView.h>

#include <boost/format.hpp>

#include <wx/gbsizer.h>
DOODLE_NAMESPACE_S

mainWindows::mainWindows()
    : wxFrame(nullptr, wxID_ANY, {"doodle"}) {
  SetIcon(wxICON(ID_DOODLE_ICON));

  // auto icon = wxIcon{};
  // icon.LoadFile("",wxBITMAP_TYPE_ICON_RESOURCE);

  SetMenuBar(new wxMenuBar{});
  CreateStatusBar(1);
  SetStatusText("doodle tools");

  auto k_parent           = new wxPanel{this, wxID_ANY};
  auto layout             = new wxBoxSizer{wxVERTICAL};
  auto k_exMaya_button    = new wxButton{k_parent, wxID_ANY, "从maya导出相机和文件"};
  auto k_create_image     = new wxButton{k_parent, wxID_ANY, "从图片创建视频"};
  auto k_create_dir_image = new wxButton{k_parent, wxID_ANY, "从多个文件夹创建视频"};
  auto k_create_video     = new wxButton{k_parent, wxID_ANY, "连接视频"};
  auto k_create_ue4File   = new wxButton{k_parent, wxID_ANY, "创建ue4关卡序列"};
  layout->Add(k_exMaya_button, wxSizerFlags{0}.Left());
  layout->Add(k_create_image, wxSizerFlags{0}.Left());
  layout->Add(k_create_dir_image, wxSizerFlags{0}.Left());
  layout->Add(k_create_video, wxSizerFlags{0}.Left());
  layout->Add(k_create_ue4File, wxSizerFlags{0}.Left());

  k_parent->SetSizer(layout);
  layout->SetSizeHints(this);

  k_exMaya_button->DragAcceptFiles(true);

  k_exMaya_button->Bind(
      wxEVT_BUTTON,
      [this](wxCommandEvent& event) {
        auto k_ = wxMessageDialog{this, "ok", "ok"};
        k_.ShowModal();
        std::cout << "ok" << std::endl;
      });
  k_exMaya_button->Bind(
      wxEVT_DROP_FILES,
      [this](wxDropFilesEvent& event) {
        if (event.GetNumberOfFiles() > 0) {
          auto k_   = wxMessageDialog{this, "file: "};
          auto k_s_ = event.GetFiles();
          k_.SetMessage(k_s_[0]);
          k_.ShowModal();
        }
      });
}

Doodle::Doodle(){

};

bool Doodle::OnInit() {
  wxApp::OnInit();
  auto k_mainWindows = new mainWindows{};
  k_mainWindows->Show();
  return true;
}

DOODLE_NAMESPACE_E
