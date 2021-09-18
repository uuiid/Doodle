//
// Created by TD on 2021/9/18.
//

#include "command.h"

#include <DoodleLib/FileWarp/MayaFile.h>
#include <DoodleLib/doodle_app.h>
#include <DoodleLib/libWarp/imgui_warp.h>

namespace doodle {
comm_export_fbx::comm_export_fbx() {
  p_name = "导出fbx";
}

bool comm_export_fbx::run() {
  if (imgui::Button("选择fbx路径")) {
    p_files.clear();
    imgui::FileDialog::Instance()->OpenModal(
        "open_get_fbx",
        "select_maya_file",
        ".ma,.mb",
        ".",
        "",
        0);
    doodle_app::Get()->main_loop.connect_extended([this](const doodle_app::connection& in) {
      dear::OpenFileDialog{"open_get_fbx"} && [in, this]() {
        auto ig = ImGuiFileDialog::Instance();
        if (ig->IsOk()) {
          auto k_paths = ig->GetSelection();
          std::transform(k_paths.begin(), k_paths.end(),
                         std::back_inserter(p_files),
                         [](const auto& j) {
                           return FSys::path{j.second} / j.second;
                         });
        }
        in.disconnect();
      };
    });
  }
  dear::ListBox{"file_list"} && [this]() {
    for (const auto& f : p_files) {
      dear::Selectable(f.generic_string());
    }
  };
  if (imgui::Button("导出")) {
    auto maya = std::make_shared<MayaFile>();
    std::for_each(p_files.begin(), p_files.end(),
                  [maya](const auto& i) { maya->exportFbxFile(i); });
  }
  return true;
}

bool comm_export_fbx::is_async() {
  return true;
}
}  // namespace doodle
