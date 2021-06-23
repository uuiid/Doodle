//
// Created by TD on 2021/5/10.
//

#include "MklinkWidget.h"

#include <DoodleLib/FileWarp/Ue4Project.h>
#include <DoodleLib/core/CoreSet.h>
#include <shellapi.h>


#include <boost/locale.hpp>
namespace doodle {
FSys::path MklinkWidget::getFilePath(wxWindow* parent) {
  auto fileDir = wxFileDialog{parent,
                              ConvStr<wxString>("get urprojrct file"),
                              wxEmptyString,
                              wxEmptyString,
                              ConvStr<wxString>(".uproject (*.uproject)|*.uproject"),
                              wxFD_FILE_MUST_EXIST};
  auto k_r     = fileDir.ShowModal();
  if (k_r == wxID_OK) {
    auto k_file = fileDir.GetPath();
    return ConvStr<std::string>(k_file);
  } else
    return {};
}

bool MklinkWidget::CreateLink() {
  // auto k_r = CoreSet::toIpPath(k_s.root_name()) / k_s.relative_path();
  //生成命令
  std::wstring com{L"-fun -mklink="};
  for (const auto& str_name : std::vector<std::string>{Ue4Project::Character, Ue4Project::Prop}) {
    auto k_s = p_source.parent_path() / Ue4Project::Content / str_name;
    auto k_t = p_target.parent_path() / Ue4Project::Content / str_name;
    if (!FSys::exists(k_s)) {
      auto k_str       = fmt::format("来源{} 不存在,跳过添加", k_s);
      auto k_wx_string = ConvStr<wxString>(k_str);
      wxMessageDialog{this, ConvStr<wxString>(k_wx_string)}.ShowModal();
      continue;
    }
    com += fmt::format(LR"({};{};)", k_s.generic_wstring(), k_t.generic_wstring());
  }
  com.pop_back();
  auto path = CoreSet::program_location() / "doodleExe.exe";

  DOODLE_LOG_INFO(boost::locale::conv::utf_to_utf<char>(com))

  auto path_  = path.generic_wstring();
  auto path_c = path_.c_str();
  auto str_c  = com.c_str();

  SHELLEXECUTEINFO ShExecInfo = {sizeof(SHELLEXECUTEINFO), 0};
  ShExecInfo.fMask            = SEE_MASK_NOCLOSEPROCESS;
  ShExecInfo.nShow            = SW_SHOWNORMAL;
  ShExecInfo.lpFile           = (LPCWSTR)path_c;
  ShExecInfo.lpParameters     = (LPCWSTR)str_c;
  ShExecInfo.lpVerb           = _T("runas");
  ShellExecuteEx(&ShExecInfo);
  WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
  CloseHandle(ShExecInfo.hProcess);
  return true;
}

MklinkWidget::MklinkWidget(wxWindow* parent, wxWindowID id)
    : wxDialog(parent, id, ConvStr<wxString>("创建连接"),
               wxDefaultPosition,
               wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
  auto layout             = new wxFlexGridSizer{3};
  auto k_soure_text       = new wxStaticText{this, wxID_ANY, ConvStr<wxString>("来源：")};
  auto k_target_text      = new wxStaticText{this, wxID_ANY, ConvStr<wxString>("目标：")};
  auto k_text_soure_ctrl  = new wxTextCtrl{this, wxID_ANY};
  auto k_text_target_ctrl = new wxTextCtrl{this, wxID_ANY};
  auto k_soure_butten     = new wxButton{this, wxID_ANY, ConvStr<wxString>("...")};
  auto k_target_butten    = new wxButton{this, wxID_ANY, ConvStr<wxString>("...")};
  auto k_ok               = new wxButton{this, wxID_ANY, ConvStr<wxString>("确认")};
  auto k_no               = new wxButton{this, wxID_ANY, ConvStr<wxString>("取消")};

  layout->Add(k_soure_text, wxSizerFlags{0}.Expand().Border(wxALL, 0))->SetProportion(0);
  layout->Add(k_text_soure_ctrl, wxSizerFlags{0}.Expand().Border(wxALL, 0))->SetProportion(4);
  layout->Add(k_soure_butten, wxSizerFlags{0}.Expand().Border(wxALL, 0))->SetProportion(0);

  layout->Add(k_target_text, wxSizerFlags{0}.Expand().Border(wxALL, 0))->SetProportion(0);
  layout->Add(k_text_target_ctrl, wxSizerFlags{0}.Expand().Border(wxALL, 0))->SetProportion(4);
  layout->Add(k_target_butten, wxSizerFlags{0}.Expand().Border(wxALL, 0))->SetProportion(0);

  layout->Add(130, 20, 0, 0, 0)->SetProportion(0);
  layout->Add(k_ok, wxSizerFlags{0}.Expand().Border(wxALL, 0))->SetProportion(4);
  layout->Add(k_no, wxSizerFlags{0}.Expand().Border(wxALL, 0))->SetProportion(0);

  //设置布局调整
  SetSizer(layout);
  //设置最小值
  layout->SetSizeHints(this);

  k_soure_butten->Bind(wxEVT_BUTTON, [k_text_soure_ctrl, this](wxCommandEvent& event) {
    auto path = getFilePath(this);
    if (!path.empty())
      this->p_source = path;
    k_text_soure_ctrl->SetValue(ConvStr<wxString>(path));
  });
  k_target_butten->Bind(wxEVT_BUTTON, [k_text_target_ctrl, this](wxCommandEvent& event) {
    auto path = getFilePath(this);
    if (!path.empty()) {
      this->p_target = path;
      k_text_target_ctrl->SetValue(ConvStr<wxString>(path));
    }
  });

  //设置为可以拖拽
  k_text_soure_ctrl->DragAcceptFiles(true);
  k_text_soure_ctrl->Bind(wxEVT_DROP_FILES, [k_text_soure_ctrl, this](wxDropFilesEvent& event) {
    auto k_num = event.GetNumberOfFiles();
    if (k_num > 0) {
      this->p_source = ConvStr<FSys::path>(event.GetFiles()[0]);
      k_text_soure_ctrl->SetValue(event.GetFiles()[0]);
    }
  });
  k_text_target_ctrl->DragAcceptFiles(true);
  k_text_target_ctrl->Bind(wxEVT_DROP_FILES, [k_text_target_ctrl, this](wxDropFilesEvent& event) {
    auto k_num = event.GetNumberOfFiles();
    if (k_num > 0) {
      this->p_target = ConvStr<FSys::path>(event.GetFiles()[0]);
      k_text_target_ctrl->SetValue(event.GetFiles()[0]);
    }
  });

  k_ok->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
    this->CreateLink();
    this->EndModal(wxID_OK);
  });
  k_no->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
    this->EndModal(wxID_CANCEL);
  });
  //调整大小
  this->Layout();
  layout->AddGrowableCol(1);
}

bool MklinkWidget::mklink(const FSys::path& in_source, const FSys::path& in_target) {
  if (FSys::exists(in_target)) {
    wxMessageDialog{nullptr, ConvStr<wxString>("已经存在重名文件， 无法添加")}.ShowModal();
    return false;
  }

  FSys::create_directory_symlink(in_source, in_target);
  auto str      = fmt::format("完成添加:\n来源:{} \n目标:{}", in_source, in_target);
  auto k_dialog = wxMessageDialog{nullptr, ConvStr<wxString>("完成添加")};
  k_dialog.SetExtendedMessage(ConvStr<wxString>(str));
  k_dialog.ShowModal();
  return true;
}
}  // namespace doodle
