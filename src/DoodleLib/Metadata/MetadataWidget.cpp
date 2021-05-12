//
// Created by TD on 2021/4/29.
//

#include <DoodleLib/Metadata/MetadataWidget.h>
#include <DoodleLib/DoodleApp.h>
#include <DoodleLib/Metadata/Model/AssDirTree.h>
#include <DoodleLib/core_Cpp.h>


namespace doodle {

MetadataWidget::MetadataWidget(wxWindow* in_window, wxWindowID in_id)
    : wxFrame(in_window, in_id, ConvStr<wxString>("Metadata")),
      p_project_ptr_(),
      p_metadata_flctory_ptr_(std::make_shared<MetadataFactory>()),
      p_tree_id_(NewControlId()),
      p_List_id_(NewControlId()),
      p_tree_view_ctrl_(),
      p_list_view_ctrl_() {

  auto k_layout = new wxBoxSizer{wxHORIZONTAL};
  p_tree_view_ctrl_ = new wxDataViewCtrl{this, p_tree_id_};
  p_list_view_ctrl_ = new wxDataViewCtrl{this, p_List_id_};

  auto k_p_text_renderer = new wxDataViewTextRenderer{"string", wxDATAVIEW_CELL_EDITABLE};
  auto k_col = new wxDataViewColumn{ConvStr<wxString>("标签树"), k_p_text_renderer, 0, 100};
  p_tree_view_ctrl_->AppendColumn(k_col);
  p_tree_view_ctrl_->AssociateModel(new AssDirTree{});

  k_p_text_renderer = new wxDataViewTextRenderer{"string",wxDATAVIEW_CELL_EDITABLE};
  k_col = new wxDataViewColumn{ConvStr<wxString>("文件"),k_p_text_renderer,0,100};
  p_list_view_ctrl_->AppendColumn(k_col);

  p_tree_view_ctrl_->SetMinSize(wxSize{300,600});
  p_list_view_ctrl_->SetMinSize(wxSize{300,600});

  k_layout->Add(p_tree_view_ctrl_,wxSizerFlags{0}.Expand().Border(wxALL,0))->SetProportion(2);
  k_layout->Add(p_list_view_ctrl_,wxSizerFlags{0}.Expand().Border(wxALL,0))->SetProportion(3);

  Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
    this->Hide();
    if (event.CanVeto())
      event.Veto(false);
  });

  SetSizer(k_layout);
  k_layout->SetSizeHints(this);
  this->Center();
}
void MetadataWidget::CreateProject() const {
  auto k_db = p_project_ptr_->Path() / Project::getConfigFileFolder() / Project::getConfigFileName();
  if (FSys::exists(k_db)) return;

  if (!FSys::exists(k_db.parent_path()))
    FSys::create_directories(k_db.parent_path());

  auto k_top_windows = wxGetApp().GetTopWindow();
  auto k_text_dialog = wxTextEntryDialog{k_top_windows, ConvStr<wxString>("项目名称: ")};
  auto k_result = k_text_dialog.ShowModal();
  if (k_result == wxID_OK) {
    auto k_text = k_text_dialog.GetValue();
    p_project_ptr_->setName(k_text);
  }
  p_project_ptr_->save(p_metadata_flctory_ptr_);
}
}  // namespace doodle
