//
// Created by TD on 2021/4/29.
//

#include <doodle_GUI/source/Metadata/MetadataWidget.h>
#include <doodle_GUI/main.h>
#include <doodle_GUI/source/Metadata/Model/AssDirTree.h>
#include <corelib/core_Cpp.h>


namespace doodle {

MetadataWidget::MetadataWidget(wxWindow* in_window, wxWindowID in_id)
    : wxFrame(in_window, in_id, ConvStr<wxString>("Metadata")),
      p_project_ptr_(),
      p_tree_id_(NewControlId()),
      p_List_id_(NewControlId()),
      p_tree_view_ctrl_(),
      p_list_view_ctrl_() {
  p_tree_view_ctrl_ = new wxDataViewCtrl{this, p_tree_id_};
  p_list_view_ctrl_ = new wxDataViewCtrl{this, p_List_id_};

  auto k_p_text_renderer = new wxDataViewTextRenderer{"string", wxDATAVIEW_CELL_EDITABLE};
  auto k_col = new wxDataViewColumn{"string", k_p_text_renderer, 0, 100};
  p_tree_view_ctrl_->AppendColumn(k_col);
  p_tree_view_ctrl_->AssociateModel(new AssDirTree{});

  k_col = new wxDataViewColumn{"string",k_p_text_renderer,0,100};
  p_list_view_ctrl_->AppendColumn(k_col);
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
  p_project_ptr_->makeProject();
}
}  // namespace doodle
