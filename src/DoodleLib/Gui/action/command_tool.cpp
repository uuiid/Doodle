//
// Created by TD on 2021/9/18.
//

#include "command_tool.h"

#include <DoodleLib/FileWarp/MayaFile.h>
#include <DoodleLib/doodle_app.h>
#include <DoodleLib/libWarp/imgui_warp.h>

namespace doodle {
comm_export_fbx::comm_export_fbx() {
  p_name = "导出fbx";
}

bool comm_export_fbx::run() {
  if (imgui::Button("选择maya文件路径")) {
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
    auto maya = new_object<maya_file_async>();
    std::for_each(p_files.begin(), p_files.end(),
                  [maya](const auto& i) { maya->export_fbx_file(i); });
  }
  return true;
}

bool comm_export_fbx::is_async() {
  return true;
}

comm_qcloth_sim::comm_qcloth_sim()
    : p_cloth_path(),
      p_text(new_object<std::string>()),
      p_sim_path(),
      p_only_sim(false) {
  p_name = "解算布料";
}
bool comm_qcloth_sim::is_async() {
  return true;
}
bool comm_qcloth_sim::run() {
  imgui::InputText("解算资产", p_text.get(), ImGuiInputTextFlags_ReadOnly);
  imgui::SameLine();
  if (imgui::Button("选择")) {
    imgui::FileDialog::Instance()->OpenModal(
        "open_get_sim_cloth_path_ass",
        "open_get_sim_cloth_path_ass",
        nullptr,
        ".",
        "",
        1);
    doodle_app::Get()->main_loop.connect_extended([this](const doodle_app::connection& in) {
      dear::OpenFileDialog{"open_get_sim_cloth_path_ass"} && [in, this]() {
        auto ig = ImGuiFileDialog::Instance();
        if (ig->IsOk()) {
          p_cloth_path = ig->GetFilePathName();
          *p_text      = p_cloth_path.generic_string();
        }
        in.disconnect();
      };
    });
  }
  if (imgui::Button("maya文件")) {
    p_sim_path.clear();
    imgui::FileDialog::Instance()->OpenModal(
        "open_get_ma",
        "select_maya_file",
        ".ma,.mb",
        ".",
        "",
        0);
    doodle_app::Get()->main_loop.connect_extended([this](const doodle_app::connection& in) {
      dear::OpenFileDialog{"open_get_ma"} && [in, this]() {
        auto ig = ImGuiFileDialog::Instance();
        if (ig->IsOk()) {
          auto k_paths = ig->GetSelection();
          std::transform(k_paths.begin(), k_paths.end(),
                         std::back_inserter(p_sim_path),
                         [](const auto& j) {
                           return FSys::path{j.second} / j.second;
                         });
        }
        in.disconnect();
      };
    });
  }

  dear::ListBox{"file_list"} && [this]() {
    for (const auto& f : p_sim_path) {
      dear::Selectable(f.generic_string());
    }
  };
  imgui::Checkbox("只解算不替换引用", &p_only_sim);
  if (imgui::Button("解算")) {
    auto maya = new_object<maya_file_async>();
    std::for_each(p_sim_path.begin(), p_sim_path.end(),
                  [this, maya](const FSys::path& in_path) {
                    auto arg                = new_object<MayaFile::qcloth_arg>();
                    arg->sim_path           = in_path;
                    arg->qcloth_assets_path = p_cloth_path;
                    arg->only_sim           = p_only_sim;
                    maya->qcloth_sim_file(arg);
                  });
  }

  return true;
}

comm_create_video::comm_create_video() {
}
bool comm_create_video::is_async() {
  return true;
}
bool comm_create_video::run() {
  return command_base::run();
}
comm_connect_video::comm_connect_video() {
}
bool comm_connect_video::is_async() {
  return true;
}
bool comm_connect_video::run() {
  return command_base::run();
}
comm_import_ue_files::comm_import_ue_files() {
}
bool comm_import_ue_files::is_async() {
  return true;
}
bool comm_import_ue_files::run() {
  return command_base::run();
}
comm_create_ue_project::comm_create_ue_project() {
}
bool comm_create_ue_project::is_async() {
  return true;
}
bool comm_create_ue_project::run() {
  return command_base::run();
}
}  // namespace doodle
