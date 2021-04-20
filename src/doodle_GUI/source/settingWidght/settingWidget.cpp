//
// Created by teXiao on 2020/10/19.
//

#include <doodle_GUI/source/SettingWidght/SettingWidget.h>

#include <loggerlib/Logger.h>

#include <corelib/core_Cpp.h>
#include <magic_enum.hpp>
#include <wx/spinctrl.h>
DOODLE_NAMESPACE_S

// static SettingWidget *doodle_SettingWidget = nullptr;

void SettingWidght::InitSetting() {
  auto& set = coreSet::getSet();
  p_dep->SetSelection(magic_enum::enum_integer<Department>(set.getDepartmentEnum()));
  auto wxstr = wxString::FromUTF8(set.getUser());
  p_user->SetValue(wxstr);
  p_cache_Text->SetLabelText(wxString::FromUTF8(set.getCacheRoot().generic_string()));
  p_Doc->SetLabelText(wxString::FromUTF8(set.getDoc().generic_string()));
  p_Maya->SetValue(wxString::FromUTF8(set.MayaPath().generic_string()));

  //设置project项目
  p_Project->Clear();
  for (auto prj : set.getAllProjects())
    p_Project->Append(prj->Name(), prj.get());
  p_Project->SetSelection(set.getProjectIndex());

  if (set.gettUe4Setting().hasPath())
    p_ue_path->SetValue(wxString::FromUTF8(set.gettUe4Setting().Path().generic_string()));

  p_ue_version->SetValue(wxString::FromUTF8(set.gettUe4Setting().Version()));
  p_ue_shot_start->SetValue(set.gettUe4Setting().ShotStart());
  p_ue_shot_end->SetValue(set.gettUe4Setting().ShotEnd());
}

SettingWidght::SettingWidght(wxWindow* parent, wxWindowID id)
    : wxFrame(parent, id, wxString::FromUTF8("设置")),
      p_dep_id(NewControlId()),
      p_user_id(NewControlId()),
      p_cache_Text_id(NewControlId()),
      p_Doc_id(NewControlId()),
      p_Maya_id(NewControlId()),
      p_Project_id(NewControlId()),
      p_Project_delete_id(NewControlId()),
      p_Project_add_id(NewControlId()),
      p_ue_path_id(NewControlId()),
      p_ue_version_id(NewControlId()),
      p_ue_shot_start_id(NewControlId()),
      p_ue_shot_end_id(NewControlId()),
      p_save_id(NewControlId()),
      p_dep(),
      p_user(),
      p_cache_Text(),
      p_Doc(),
      p_Maya(),
      p_Project(),
      p_ue_path(),
      p_ue_version(),
      p_ue_shot_start(),
      p_ue_shot_end() {
  auto& set = coreSet::getSet();
  //创建布局
  auto layout = new wxFlexGridSizer{4};
  layout->AddGrowableCol(1);
  //部门
  layout->Add(new wxStaticText{this, wxID_ANY, wxString::FromUTF8("部门: ")}, wxSizerFlags{0}.Expand());
  wxArrayString k_deps{};
  for (auto str : magic_enum::enum_names<Department>()) {
    k_deps.Add(wxString::FromUTF8(std::string{str}));
  }
  p_dep = new wxComboBox{this, p_dep_id,
                         wxString::FromUTF8("VFX"), wxDefaultPosition, wxDefaultSize, k_deps};
  layout->Add(p_dep, wxSizerFlags{0}.Expand().Border(wxALL, 0));
  layout->Add(130, 20, 0, 0, 0);
  layout->Add(130, 20, 0, 0, 0);
  //用户
  layout->Add(new wxStaticText{this, wxID_ANY, wxString::FromUTF8("用户名称: ")}, wxSizerFlags{0}.Expand());
  p_user = new wxTextCtrl{this, p_user_id, wxString::FromUTF8("usre")};
  layout->Add(p_user, wxSizerFlags{0}.Expand());
  layout->Add(130, 20, 0, 0, 0);
  layout->Add(130, 20, 0, 0, 0);

  //缓存路径
  layout->Add(new wxStaticText{this, wxID_ANY, wxString::FromUTF8("缓存路径: ")}, wxSizerFlags{0}.Expand());
  p_cache_Text = new wxStaticText{this, p_cache_Text_id,
                                  wxString::FromUTF8(set.getCacheRoot().generic_string())};
  layout->Add(p_cache_Text,
              wxSizerFlags{0}.Expand());
  layout->Add(130, 20, 0, 0, 0);
  layout->Add(130, 20, 0, 0, 0);

  //文档路径
  layout->Add(new wxStaticText{this, wxID_ANY, wxString::FromUTF8("文档路径: ")}, wxSizerFlags{0}.Expand());
  p_Doc = new wxStaticText{this, p_Doc_id, wxString::FromUTF8(set.getDoc().generic_string())};
  layout->Add(p_Doc,
              wxSizerFlags{0}.Expand());
  layout->Add(130, 20, 0, 0, 0);
  layout->Add(130, 20, 0, 0, 0);
  //maya路径
  layout->Add(new wxStaticText{this, wxID_ANY, wxString::FromUTF8("maya路径: ")}, wxSizerFlags{0}.Expand());
  p_Maya = new wxTextCtrl{this, p_Maya_id};
  layout->Add(p_Maya, wxSizerFlags{0}.Expand());
  auto k_maya_butten = new wxButton{this, wxID_ANY, wxString::FromUTF8("...")};
  layout->Add(k_maya_butten, wxSizerFlags{0}.Expand());
  layout->Add(130, 20, 0, 0, 0);

  //项目列表
  layout->Add(new wxStaticText{this, wxID_ANY, wxString::FromUTF8("项目: ")}, wxSizerFlags{0}.Expand());
  p_Project = new wxComboBox{this, p_Project_id};
  layout->Add(p_Project, wxSizerFlags{0}.Expand());
  //添加和删除项目按钮
  auto k_Project_delete = new wxButton{this, p_Project_delete_id, wxString::FromUTF8("删除")};
  layout->Add(k_Project_delete, wxSizerFlags{0}.Expand());
  auto k_Project_add = new wxButton{this, p_Project_add_id, wxString::FromUTF8("添加")};
  layout->Add(k_Project_add, wxSizerFlags{0}.Expand());

  //ue安装路径
  layout->Add(new wxStaticText{this, wxID_ANY, wxString::FromUTF8("ue安装路径")}, wxSizerFlags{0}.Expand());
  p_ue_path = new wxTextCtrl{this, p_ue_path_id};
  layout->Add(p_ue_path, wxSizerFlags{0}.Expand());
  auto k_ue_butten = new wxButton{this, wxID_ANY, wxString::FromUTF8("...")};
  layout->Add(k_ue_butten, wxSizerFlags{0}.Expand());
  layout->Add(130, 20, 0, 0, 0);

  //ue查找版本
  layout->Add(new wxStaticText{this, wxID_ANY, wxString::FromUTF8("ue查找版本")}, wxSizerFlags{0}.Expand());
  p_ue_version = new wxTextCtrl{this, p_ue_path_id, wxString::FromUTF8(set.gettUe4Setting().Version())};
  layout->Add(p_ue_version, wxSizerFlags{0}.Expand());
  layout->Add(130, 20, 0, 0, 0);
  layout->Add(130, 20, 0, 0, 0);
  if (set.gettUe4Setting().hasPath())
    p_ue_version->Enable(false);

  //ue镜头开始
  layout->Add(new wxStaticText{this, wxID_ANY, wxString::FromUTF8("默认镜头开始")}, wxSizerFlags{0}.Expand());
  p_ue_shot_start = new wxSpinCtrl{this, p_ue_shot_start_id};
  p_ue_shot_start->SetRange(0, 9999);
  layout->Add(p_ue_shot_start, wxSizerFlags{0}.Expand());
  layout->Add(130, 20, 0, 0, 0);
  layout->Add(130, 20, 0, 0, 0);

  //ue镜头结束
  layout->Add(new wxStaticText{this, wxID_ANY, wxString::FromUTF8("默认镜头结束")}, wxSizerFlags{0}.Expand());
  p_ue_shot_end = new wxSpinCtrl{this, p_ue_shot_end_id};
  p_ue_shot_end->SetRange(0, 9999);
  layout->Add(p_ue_shot_end, wxSizerFlags{0}.Expand());
  layout->Add(130, 20, 0, 0, 0);
  layout->Add(130, 20, 0, 0, 0);

  //保存按钮
  layout->Add(130, 20, 0, 0, 0);
  auto k_save = new wxButton{this, p_save_id, wxString::FromUTF8("测试数据并保存")};
  k_save->SetBackgroundColour(wxColourDatabase().Find(wxString::FromUTF8("MEDIUM VIOLET RED")));
  layout->Add(k_save, wxSizerFlags{0}.Expand());
  layout->Add(130, 20, 0, 0, 0);
  layout->Add(130, 20, 0, 0, 0);
  this->InitSetting();

  SetSizer(layout);
  layout->SetSizeHints(this);

  //绑定回调
  p_dep->Bind(wxEVT_COMBOBOX, [&set, this](wxCommandEvent& event) {
    set.setDepartment(p_dep->GetValue().ToStdString(wxConvUTF8));
  });
  p_user->Bind(wxEVT_TEXT, [&set, this](wxCommandEvent& event) {
    set.setUser(p_user->GetValue().ToStdString(wxConvUTF8));
  });
  //maya路径
  p_Maya->Bind(wxEVT_TEXT, [&set, this](wxCommandEvent& event) {
    set.setMayaPath(p_Maya->GetValue().ToStdString(wxConvUTF8));
  });
  k_maya_butten->Bind(wxEVT_BUTTON, [&set, this](wxCommandEvent& event) {
    auto k_dir_dig = wxDirDialog{this, wxString::FromUTF8("maya路径")};
    if (k_dir_dig.ShowModal() != wxID_OK)
      return;
    auto path_str = k_dir_dig.GetPath();
    p_Maya->SetValue(path_str);
    FSys::path path{path_str.ToStdString(wxConvUTF8)};
    set.setMayaPath(path);
  });

  p_Project->Bind(wxEVT_COMBOBOX, [&set, this](wxCommandEvent& event) {
    auto index = p_Project->GetSelection();
    auto prj   = p_Project->GetClientData(index);
    set.setProject_(reinterpret_cast<Project*>(prj));
  });
  p_ue_path->Bind(wxEVT_TEXT, [&set, this](wxCommandEvent& event) {
    auto value = p_ue_path->GetValue();
    p_ue_version->Enable(value.empty());
    set.gettUe4Setting().setPath(value.ToStdString(wxConvUTF8));
  });
  k_ue_butten->Bind(wxEVT_BUTTON, [&set, this](wxCommandEvent& event) {
    auto k_dir_dig = wxDirDialog{this, wxString::FromUTF8("ue路径")};
    if (k_dir_dig.ShowModal() != wxID_OK)
      return;
    auto path_str = k_dir_dig.GetPath();
    p_ue_path->SetValue(path_str);
    FSys::path path{path_str.ToStdString(wxConvUTF8)};
    set.gettUe4Setting().setPath(path);
    p_ue_version->Enable(false);
  });

  p_ue_version->Bind(wxEVT_TEXT, [&set, this](wxCommandEvent& event) {
    auto value = p_ue_version->GetValue();
    set.gettUe4Setting().setVersion(value.ToStdString(wxConvUTF8));
  });

  p_ue_shot_start->Bind(wxEVT_SPINCTRL, [&set, this](wxCommandEvent& event) {
    auto value = p_ue_shot_start->GetValue();
    set.gettUe4Setting().setShotStart(value);
  });
  p_ue_shot_end->Bind(wxEVT_SPINCTRL, [&set, this](wxCommandEvent& event) {
    auto value = p_ue_shot_end->GetValue();
    set.gettUe4Setting().setShotEnd(value);
  });
  k_save->Bind(wxEVT_BUTTON, [&set, this](wxCommandEvent& event) {
    try {
      set.writeDoodleLocalSet();
    } catch (const std::exception& error) {
      wxMessageDialog{this, error.what(), wxString::FromUTF8("错误")}.ShowModal();
    }
    this->InitSetting();
  });

  k_Project_add->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) { this->AddProject(); });
  k_Project_delete->Bind(wxEVT_BUTTON, [&set, this](wxCommandEvent& event) {
    auto value = p_Project->GetSelection();
    auto prj   = p_Project->GetClientData(value);

    set.deleteProject(reinterpret_cast<Project*>(prj));
    p_Project->Delete(value);
    auto selection = value - 1;
    selection      = selection > 0 ? selection : 0;
    p_Project->SetSelection(selection);
    set.setProject_(reinterpret_cast<Project*>(p_Project->GetClientData(selection)));
  });

  Bind(wxEVT_CLOSE_WINDOW, [&set, this](wxCloseEvent& event) {
    try {
      set.writeDoodleLocalSet();
    } catch (const std::exception& error) {
      wxMessageDialog{this, error.what(), wxString::FromUTF8("错误")}.ShowModal();
    }

    this->Hide();
    if (event.CanVeto())
      event.Veto(false);
  });
}

void SettingWidght::AddProject() {
  auto path_dialog = wxDirDialog{this, wxString::FromUTF8("选择项目根目录: "), wxEmptyString, wxRESIZE_BORDER};
  auto result      = path_dialog.ShowModal();
  if (result == wxID_OK) {
    auto path        = path_dialog.GetPath();
    auto text_dialog = wxTextEntryDialog{this, wxString::FromUTF8("项目名称: ")};
    result           = text_dialog.ShowModal();
    if (result == wxID_OK) {
      auto text = text_dialog.GetValue();
      if (!text.empty()) {
        auto prj = std::make_shared<Project>(path.ToStdString(wxConvUTF8), text);
        coreSet::getSet().installProject(prj);
        p_Project->Append(text, prj.get());
      }
    }
  }
}

DOODLE_NAMESPACE_E