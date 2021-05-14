//
// Created by TD on 2021/4/29.
//

#include <DoodleLib/Metadata/MetadataWidget.h>
#include <DoodleLib/DoodleApp.h>
#include <DoodleLib/Metadata/Model/AssDirTree.h>
#include <DoodleLib/core_Cpp.h>
#include <DoodleLib/Metadata/MetadataFactory.h>

namespace doodle {

MetadataWidget::MetadataWidget(wxWindow* in_window, wxWindowID in_id)
    : wxFrame(in_window, in_id, ConvStr<wxString>("Metadata")),
      p_project_ptr_(),
      p_metadata_flctory_ptr_(std::make_shared<MetadataFactory>()),
      p_tree_id_(NewControlId()),
      p_List_id_(NewControlId()),
      p_tree_view_ctrl_(),
      p_list_view_ctrl_() {
  auto k_layout     = new wxBoxSizer{wxHORIZONTAL};
  p_tree_view_ctrl_ = new wxDataViewCtrl{this, p_tree_id_};
  p_list_view_ctrl_ = new wxDataViewCtrl{this, p_List_id_};

  auto k_p_text_renderer = new wxDataViewTextRenderer{"string", wxDATAVIEW_CELL_EDITABLE};
  auto k_col             = new wxDataViewColumn{ConvStr<wxString>("标签树"), k_p_text_renderer, 0, 100};
  p_tree_view_ctrl_->AppendColumn(k_col);
  p_tree_view_ctrl_->AssociateModel(new AssDirTree{});
  k_p_text_renderer = new wxDataViewTextRenderer{"string", wxDATAVIEW_CELL_EDITABLE};
  k_col             = new wxDataViewColumn{ConvStr<wxString>("文件"), k_p_text_renderer, 0, 100};
  p_list_view_ctrl_->AppendColumn(k_col);
  p_tree_view_ctrl_->SetMinSize(wxSize{300, 600});
  p_list_view_ctrl_->SetMinSize(wxSize{300, 600});

  k_layout->Add(p_tree_view_ctrl_, wxSizerFlags{0}.Expand().Border(wxALL, 0))->SetProportion(2);
  k_layout->Add(p_list_view_ctrl_, wxSizerFlags{0}.Expand().Border(wxALL, 0))->SetProportion(3);

  //绑定各种信号
  p_tree_view_ctrl_->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,&MetadataWidget::treeContextMenu,this);
  p_list_view_ctrl_->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,&MetadataWidget::listContextMenu,this);
  //关闭时隐藏，不销毁
  Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
    this->Hide();
    if (event.CanVeto())
      event.Veto(false);
  });

  SetSizer(k_layout);
  k_layout->SetSizeHints(this);
  this->Center();
}
void MetadataWidget::CreateProject()  {
  auto k_db = p_project_ptr_->Path() / Project::getConfigFileFolder() / Project::getConfigFileName();
  if (FSys::exists(k_db)) return;

  if (!FSys::exists(k_db.parent_path()))
    FSys::create_directories(k_db.parent_path());

  auto k_top_windows = wxGetApp().GetTopWindow();
  auto k_text_dialog = wxTextEntryDialog{k_top_windows, ConvStr<wxString>("项目名称: ")};
  auto k_result      = k_text_dialog.ShowModal();
  if (k_result == wxID_OK) {
    auto k_text = k_text_dialog.GetValue();
    p_project_ptr_->setName(k_text);
  }
  p_project_ptr_->save(p_metadata_flctory_ptr_);
}
void MetadataWidget::AddProject() {
  auto path_dialog = wxDirDialog{this, ConvStr<wxString>("选择项目根目录: "), wxEmptyString, wxRESIZE_BORDER};
  auto result      = path_dialog.ShowModal();
  if (result != wxID_OK) return;
  auto path = ConvStr<FSys::path>(path_dialog.GetPath());
  if (path.empty()) return;
  auto prj = std::make_shared<Project>(path);
  prj->load(p_metadata_flctory_ptr_);
  MetadataSet::Get().installProject(prj);
}
void MetadataWidget::deleteProject()  {
  auto k_item = p_tree_view_ctrl_->GetCurrentItem();
  if (!k_item.IsOk())
    return;
  auto k_root = reinterpret_cast<Project*>(k_item.GetID());
  if (k_root)
    MetadataSet::Get().deleteProject(k_root);
}
void MetadataWidget::treeContextMenu(wxDataViewEvent& in_event) {
  wxWindowIDRef k_add_id{NewControlId()};
  wxWindowIDRef k_delete_id{NewControlId()};
  wxWindowIDRef k_create_id{NewControlId()};

  wxMenu k_wx_menu{};
  k_wx_menu.Append(k_add_id, ConvStr<wxString>("添加项目"));
  PopupMenu(&k_wx_menu);
}
void MetadataWidget::listContextMenu(wxDataViewEvent& in_event) {

}

}  // namespace doodle
