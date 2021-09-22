//
// Created by TD on 2021/9/18.
//

#include "command_tool.h"

#include <DoodleLib/FileWarp/ImageSequence.h>
#include <DoodleLib/FileWarp/MayaFile.h>
#include <DoodleLib/FileWarp/VideoSequence.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/Shot.h>
#include <DoodleLib/doodle_app.h>
#include <DoodleLib/libWarp/imgui_warp.h>

#include <DoodleLib/core/open_file_dialog.h>

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
          auto k_paths     = ig->GetSelection();
          FSys::path k_dir = ig->GetCurrentPath();
          std::transform(k_paths.begin(), k_paths.end(),
                         std::back_inserter(p_files),
                         [&k_dir](const auto& j) {
                           return k_dir / j.second;
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
          auto k_paths     = ig->GetSelection();
          FSys::path k_dir = ig->GetCurrentPath();
          std::transform(k_paths.begin(), k_paths.end(),
                         std::back_inserter(p_sim_path),
                         [&k_dir](const auto& j) {
                           return k_dir / j.second;
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

comm_create_video::comm_create_video()
    : p_video_path(),
      p_image_path(),
      p_out_path(new_object<std::string>()) {
  p_name = "创建视频";
}
bool comm_create_video::is_async() {
  return true;
}
bool comm_create_video::run() {
  imgui::InputText("输出文件夹", p_out_path.get());
  imgui::SameLine();
  if (imgui::Button("选择")) {
    imgui::FileDialog::Instance()->OpenModal(
        "comm_create_video",
        "选择目录",
        nullptr,
        ".",
        "",
        1);
    doodle_app::Get()->main_loop.connect_extended([this](const doodle_app::connection& in) {
      dear::OpenFileDialog{"comm_create_video"} && [in, this]() {
        auto ig = ImGuiFileDialog::Instance();
        if (ig->IsOk()) {
          *p_out_path = ig->GetFilePathName();
        }
        in.disconnect();
      };
    });
  }

  if (imgui::Button("选择图片")) {
    imgui::FileDialog::Instance()->OpenModal(
        "select_image_comm_create_video",
        "选择序列",
        ".png,.jpg",
        ".",
        "",
        0);
    doodle_app::Get()->main_loop.connect_extended([this](const doodle_app::connection& in) {
      dear::OpenFileDialog{"select_image_comm_create_video"} && [in, this]() {
        auto ig = ImGuiFileDialog::Instance();
        if (ig->IsOk()) {
          image_paths k_image_paths{};
          k_image_paths.use_dir = false;
          auto k_path           = ig->GetSelection();
          FSys::path k_dir      = ig->GetCurrentPath();
          std::transform(k_path.begin(),
                         k_path.end(),
                         std::back_inserter(k_image_paths.p_path_list),
                         [&k_dir](const auto& j) {
                           return k_dir / j.second;
                         });
          k_image_paths.p_show_name = k_image_paths.p_path_list.front().parent_path().generic_string();
          p_image_path.emplace_back(std::move(k_image_paths));
        }
        in.disconnect();
      };
    });
  }
  imgui::SameLine();
  if (imgui::Button("选择文件夹")) {
    open_file_dialog{"comm_create_video",
                     "select dir",
                     nullptr,
                     ".",
                     "",
                     0}.show(
            [this](const FSys::path& in){
              image_paths k_image_paths{};
              k_image_paths.use_dir = true;
              k_image_paths.p_path_list.emplace_back(in);
              k_image_paths.p_show_name = fmt::format("{}###{}",
                                                      k_image_paths.p_path_list.front().generic_string(),
                                                      fmt::ptr(&k_image_paths));
              p_image_path.emplace_back(std::move(k_image_paths));
            }
            );
    });
  }
  imgui::SameLine();
  if (imgui::Button("清除")) {
    p_image_path.clear();
  }
  imgui::SameLine();
  if (imgui::Button("创建视频")) {
    auto image = new_object<image_sequence_async>();
    for (const auto& i : p_image_path) {
      ImageSequencePtr ptr{};
      if (i.use_dir) {
        ptr = image->set_path(i.p_path_list.front());
      } else {
        ptr = image->set_path(i.p_path_list);
      }
      ptr->set_out_dir(*p_out_path);
      ptr->set_shot_and_eps(Shot::analysis_static(i.p_path_list.front()),
                            Episodes::analysis_static(i.p_path_list.front()));

      image->create_video(*p_out_path);
    }
  }

  dear::ListBox{"image_list"} && [this]() {
    for (const auto& i : p_image_path) {
      dear::Selectable(i.p_show_name);
    }
  };

  if (imgui::Button("选择视频")) {
//    open_file_dialog{"comm_create_video",
//                       "select dir",
//                       nullptr,
//                       ".",
//                       "",
//                       0}
  }
  imgui::SameLine();
  if (imgui::Button("连接视频")) {
    auto video = new_object<video_sequence_async>();
    video->set_video_list(p_video_path);
    video->connect_video(*p_out_path);
  }

  dear::ListBox{"video_list"} && [this]() {
    for (const auto& i : p_video_path) {
      dear::Selectable(i.filename().generic_string());
    }
  };

  return true;
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
