//
// Created by TD on 2021/4/29.
//

#include <doodle_GUI/source/Metadata/MetadataWidget.h>
#include <corelib/core_Cpp.h>
#include <doodle_GUI/source/mainWidght/mainWindows.h>

namespace doodle {

MetadataWidget::MetadataWidget() {
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
