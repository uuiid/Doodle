//
// Created by TD on 2021/9/18.
//

#include "command_meta.h"

#include <DoodleLib/Metadata/Metadata_cpp.h>
#include <DoodleLib/core/DoodleLib.h>
#include <DoodleLib/core/open_file_dialog.h>
#include <DoodleLib/libWarp/imgui_warp.h>

namespace doodle {
comm_project_add::comm_project_add()
    : p_prj_name(new_object<string>()),
      p_prj_name_short(new_object<string>()),
      p_prj_path(new_object<string>()),
      p_root() {
  p_name = "项目";
}

bool comm_project_add::render() {
  auto& k_d_lib = DoodleLib::Get();
  if (imgui::Button("添加")) {
    auto k_prj = new_object<Project>(*p_prj_path, *p_prj_name);
    k_prj->updata_db(k_d_lib.get_metadata_factory());
    k_d_lib.p_project_vector = k_d_lib.get_metadata_factory()->getAllProject();
  }
  if (p_root) {
    imgui::SameLine();
    if (imgui::Button("修改")) {
      p_root->setName(*p_prj_name);
      p_root->setPath(*p_prj_path);
      p_root->updata_db();
    }
    imgui::SameLine();
    if (imgui::Button("删除")) {
      p_root->deleteData();
      k_d_lib.p_project_vector = k_d_lib.get_metadata_factory()->getAllProject();
    }
  }

  imgui::InputText("名称", p_prj_name.get());
  imgui::InputText("路径", p_prj_path.get());
  imgui::SameLine();
  if (imgui::Button("选择")) {
    open_file_dialog{"open_select_path",
                     "选择路径",
                     nullptr,
                     ".",
                     "",
                     1}
        .show(
            [this](const std::vector<FSys::path>& in) {
              *p_prj_path = in.front().generic_string();
            });
  }

  return true;
}

bool comm_project_add::add_data(const MetadataPtr& in_parent, const MetadataPtr& in) {
  p_root = std::dynamic_pointer_cast<Project>(in);
  if (p_root) {
    *p_prj_name = p_root->getName();
    *p_prj_path = p_root->getPath().generic_string();
  }
  return p_root != nullptr;
}

}  // namespace doodle
