#include <doodle_GUI/source/mainWidght/mainWindows.h>
//logger是boost库使用者，放到qt上面能好点
#include <loggerlib/Logger.h>

#include <corelib/core_Cpp.h>

#include <doodle_GUI/source/mainWidght/FileDropTarget.h>
#include <doodle_GUI/source/mainWidght/systemTray.h>
#include <doodle_GUI/source/toolkit/MessageAndProgress.h>
#include <doodle_GUI/source/SettingWidght/SettingWidget.h>
#include <doodle_GUI/source/Metadata/View/ShotListView.h>

#include <boost/format.hpp>

#include <wx/gbsizer.h>

#include <loggerlib/Logger.h>
DOODLE_NAMESPACE_S

mainWindows::mainWindows()
    : wxFrame(nullptr, wxID_ANY, {"doodle"}),
      p_exmaya_id(NewControlId()),
      p_create_image_id(NewControlId()),
      p_create_dir_image_id(NewControlId()),
      p_create_video_id(NewControlId()),
      p_create_ue4File_id(NewControlId()) {
  // SetMenuBar(new wxMenuBar{});
  // CreateStatusBar(1);
  // SetStatusText("doodle tools");

  auto k_parent = new wxPanel{this, wxID_ANY};
  //创建总布局
  auto layout = new wxBoxSizer{wxVERTICAL};
  //创建按钮
  auto k_exMaya_button    = new wxButton{k_parent, p_exmaya_id, _((wxString::FromUTF8("从maya导出相机和文件")))};
  auto k_create_image     = new wxButton{k_parent, p_create_image_id, _((wxString::FromUTF8("从图片创建视频")))};
  auto k_create_dir_image = new wxButton{k_parent, p_create_dir_image_id, _((wxString::FromUTF8("从多个文件夹创建视频")))};
  auto k_create_video     = new wxButton{k_parent, p_create_video_id, _((wxString::FromUTF8("连接视频")))};
  auto k_create_ue4File   = new wxButton{k_parent, p_create_ue4File_id, _((wxString::FromUTF8("创建ue4关卡序列")))};
  //布局
  layout->Add(k_exMaya_button, wxSizerFlags{0}.Expand().Left());
  layout->Add(k_create_image, wxSizerFlags{0}.Expand().Left());
  layout->Add(k_create_dir_image, wxSizerFlags{0}.Expand().Left());
  layout->Add(k_create_video, wxSizerFlags{0}.Expand().Left());
  layout->Add(k_create_ue4File, wxSizerFlags{0}.Expand().Left());

  //设置布局调整
  k_parent->SetSizer(layout);
  //设置最小值
  layout->SetSizeHints(this);

  //可以拖拽文件
  k_exMaya_button->DragAcceptFiles(true);
  auto k_dray = new FileDropTarget{};
  k_dray->handleFileFunction.connect([this](const std::vector<FSys::path> paths) {
    auto maya    = std::make_shared<MayaFile>();
    auto process = new MessageAndProgress{this};
    process->createProgress(maya);

    std::thread{
        [maya, paths]() { maya->batchExportFbxFile(paths); }}
        .detach();
  });
  k_exMaya_button->SetDropTarget(k_dray);
  k_create_image->DragAcceptFiles(true);
  k_create_dir_image->DragAcceptFiles(true);
  k_create_video->DragAcceptFiles(true);
  k_create_ue4File->DragAcceptFiles(true);
}

void mainWindows::exportMayaFile(const std::vector<FSys::path> paths) {
  auto maya    = std::make_shared<MayaFile>();
  auto process = new MessageAndProgress{this};
  process->createProgress(maya);

  std::thread{
      [maya, paths]() { maya->batchExportFbxFile(paths); }}
      .detach();
}

void mainWindows::createVideoFile(const std::vector<FSys::path> paths) {
  auto image   = std::make_shared<ImageSequence>(paths);
  auto process = new MessageAndProgress{this};

  process->createProgress(image);
  auto path = paths.at(0).parent_path() / image->getEpisodesAndShot_str().append(".mp4");
  std::thread{
      [image, path]() {
        image->createVideoFile(path);
      }}
      .detach();
}

void mainWindows::createVideoFileFormDir(const std::vector<FSys::path> paths) {
  auto bath    = std::make_shared<ImageSequenceBatch>(paths);
  auto process = new MessageAndProgress{this};

  process->createProgress(bath);
  std::thread{
      [bath] {
        bath->batchCreateSequence();
      }}
      .detach();
}

void mainWindows::connectVideo(const std::vector<FSys::path> paths) {
  auto data    = std::make_shared<VideoSequence>(paths);
  auto process = new MessageAndProgress{this};

  process->createProgress(data);
  std::thread{
      [data] {
        data->connectVideo();
      }}
      .detach();
}

void mainWindows::createUe4Project(const std::vector<FSys::path> paths) {
  auto ue = std::make_shared<Ue4Project>(paths[0]);
  // auto [k_eps, k_shot] = ShotListDialog::getShotList(this);
  // if (k_shot.empty()) {
  //   QMessageBox::warning(this, QString{"注意:"}, tr("取消创建"));
  //   return;
  // }
  // ue->createShotFolder(k_shot);
}

std::vector<FSys::path> mainWindows::convertDropFile(wxDropFilesEvent& event) {
  std::vector<FSys::path> list;

  const auto nums = event.GetNumberOfFiles();
  if (nums > 0) {
    // auto str = event.GetFiles();
    for (int i = 0; i < nums; ++i) {
      // auto wxstr = event.GetFiles()[i];
      auto k_ = wxMessageDialog{this, "ok", event.GetFiles()[i]};
      k_.ShowModal();
      // auto stdstr = wxstr.ToStdString();
      // auto buff = wxstr.mb_str();
      // std::string str_{wxstr};
      // auto str_ = std::string{buff.data(), buff.length()};
      // list.emplace_back(FSys::path{wxstr});
    }
  }
  return list;
}

Doodle::Doodle()
    : p_mainWindwos(nullptr),
      p_setting_widget(nullptr) {
  ;
};

int Doodle::OnExit() {
  // if (p_mainWindwos) {
  //   p_mainWindwos->Destroy();
  //   delete p_mainWindwos;
  // }
  if (p_systemTray)
    delete p_systemTray;
  return 0;
}

void Doodle::openMainWindow() {
  p_mainWindwos->Show();
}

bool Doodle::OnInit() {
  wxApp::OnInit();
  wxLog::EnableLogging(false);

  // AllocConsole();
  // // redirect unbuffered STDOUT to the console
  // auto lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
  // auto hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
  // auto fp = _fdopen(hConHandle, "w");
  // *stdout = *fp;
  // setvbuf(stdout, NULL, _IONBF, 0);
  // // redirect unbuffered STDIN to the console
  // lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
  // hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
  // fp = _fdopen(hConHandle, "r");
  // *stdin = *fp;
  // setvbuf(stdin, NULL, _IONBF, 0);
  // // redirect unbuffered STDERR to the console
  // lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
  // hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
  // fp = _fdopen(hConHandle, "w");
  // *stderr = *fp;
  // setvbuf(stderr, NULL, _IONBF, 0);
  // // make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
  // // point to console as well
  // std::ios::sync_with_stdio();

  p_mainWindwos = new mainWindows{};
  p_mainWindwos->SetIcon(wxICON(ID_DOODLE_ICON));

  p_systemTray = new systemTray{};
  p_systemTray->SetIcon(wxICON(ID_DOODLE_ICON));
  p_mainWindwos->Show();

  return true;
}

DOODLE_NAMESPACE_E
