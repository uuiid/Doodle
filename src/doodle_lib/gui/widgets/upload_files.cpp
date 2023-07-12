//
// Created by td_main on 2023/7/12.
//

#include "upload_files.h"

#include "imgui_stdlib.h"

namespace doodle::gui {
const std::string& upload_files::title() const { return title_; }
bool upload_files::render() {
  if (ImGui::InputText(*ue_file_, &ue_file_)) {
  }
  if (ImGui::InputText(*maya_file_, &maya_file_)) {
  }
  if (ImGui::InputText(*rig_file_, &rig_file_)) {
  }

  return show_;
}
}  // namespace doodle::gui