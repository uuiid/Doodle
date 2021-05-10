//
// Created by TD on 2021/5/10.
//

#include "MklinkWidget.h"

#include <DoodleLib/FileWarp/Ue4Project.h>
#include <DoodleLib/core/coreset.h>
#include <boost/format.hpp>

#include <shellapi.h>
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
  auto k_s = p_source.parent_path() / Ue4Project::Content / Ue4Project::Character;
  auto k_t = p_target.parent_path() / Ue4Project::Content / Ue4Project::Character;

  if (FSys::exists(k_t.parent_path()))
    FSys::create_directories(k_t.parent_path());

  if (FSys::exists(k_s) && !FSys::exists(k_t)) {
    auto k_r = CreateSymbolicLink(k_s.generic_wstring().c_str(), k_t.generic_wstring().c_str(), SYMBOLIC_LINK_FLAG_DIRECTORY);
    if (k_r == 0) {
      auto k_err = GetLastError();
      wxMessageDialog{this, ConvStr<wxString>("出错了!(请提升为管理员试试)")}.ShowModal();
      return false;
    }
    return true;
  } else
    wxMessageDialog{this, ConvStr<wxString>("来源不存在或者目标已存在! ")}.ShowModal();
  return false;
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
    auto path      = getFilePath(this);
    this->p_source = path;
    k_text_soure_ctrl->SetValue(ConvStr<wxString>(path));
  });
  k_target_butten->Bind(wxEVT_BUTTON, [k_text_target_ctrl, this](wxCommandEvent& event) {
    auto path      = getFilePath(this);
    this->p_target = path;
    k_text_target_ctrl->SetValue(ConvStr<wxString>(path));
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

bool MklinkWidget::mklink(wxWindow* parent) {
  // auto mk  = MklinkWidget{parent, wxID_ANY};
  // auto k_r = mk.ShowModal();
  // return k_r == wxID_OK;
  //生成命令
  boost::wformat str{LR"(-fun=mklink)"};
  auto path = coreSet::getSet().program_location() / "doodleExe.exe";
  // str % k_tmp_path.generic_wstring() % file_path.generic_wstring() % k_export_path.generic_wstring();
  // str % p_path.generic_wstring();
  // boost::format str{R"(%1% --path %2% --exportpath %3%)"};
  // str % k_tmp_path.generic_string() % file_path.generic_string() % k_export_path.generic_string();

  DOODLE_LOG_INFO(str.str())

  auto path_  = path.generic_wstring();
  auto path_c = path_.c_str();
  auto str_   = str.str();
  auto str_c  = str_.c_str();

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
}  // namespace doodle