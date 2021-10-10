//
// Created by TD on 2021/9/18.
//

#include "command_tool.h"

#include <doodle_lib/core/open_file_dialog.h>
#include <doodle_lib/doodle_app.h>
#include <doodle_lib/file_warp/image_sequence.h>
#include <doodle_lib/file_warp/maya_file.h>
#include <doodle_lib/file_warp/ue4_project.h>
#include <doodle_lib/file_warp/video_sequence.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/episodes.h>
#include <doodle_lib/metadata/shot.h>

#include <boost/assign.hpp>
#include <boost/range.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
namespace doodle {

comm_export_fbx::comm_export_fbx() {
  p_name = "导出fbx";
}

bool comm_export_fbx::render() {
  if (imgui::Button("选择maya文件路径")) {
    p_files.clear();
    open_file_dialog{
        "open_get_fbx",
        "select_maya_file",
        ".ma,.mb",
        ".",
        "",
        0}
        .show(
            [this](const std::vector<FSys::path>& in_p) {
              p_files = in_p;
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
bool comm_qcloth_sim::render() {
  imgui::InputText("解算资产", p_text.get(), ImGuiInputTextFlags_ReadOnly);
  imgui::SameLine();
  if (imgui::Button("选择")) {
    open_file_dialog{
        "open_get_sim_cloth_path_ass",
        "open_get_sim_cloth_path_ass",
        nullptr,
        ".",
        "",
        1}
        .show(
            [this](const std::vector<FSys::path>& in_p) {
              p_cloth_path = in_p.front();
              *p_text      = p_cloth_path.generic_string();
            });
  }
  if (imgui::Button("maya文件")) {
    p_sim_path.clear();
    open_file_dialog{
        "open_get_ma",
        "select_maya_file",
        ".ma,.mb",
        ".",
        "",
        0}
        .show(
            [this](const std::vector<FSys::path>& in_p) {
              p_sim_path = in_p;
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
                    auto arg                = new_object<maya_file::qcloth_arg>();
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
bool comm_create_video::render() {
  imgui::InputText("输出文件夹", p_out_path.get());
  imgui::SameLine();
  if (imgui::Button("选择")) {
    open_file_dialog{
        "comm_create_video",
        "选择目录",
        nullptr,
        ".",
        "",
        1}
        .show(
            [this](const std::vector<FSys::path>& in_p) {
              if (!in_p.empty())
                *p_out_path = in_p.front().generic_string();
            });
  }

  if (imgui::Button("选择图片")) {
    open_file_dialog{
        "select_image_comm_create_video",
        "选择序列",
        ".png,.jpg",
        ".",
        "",
        0}
        .show([this](const std::vector<FSys::path>& in) {
          image_paths k_image_paths{};
          k_image_paths.use_dir     = false;
          k_image_paths.p_path_list = in;
          k_image_paths.p_show_name = k_image_paths.p_path_list.front().parent_path().generic_string();
          p_image_path.emplace_back(std::move(k_image_paths));
        });
  }
  imgui::SameLine();
  if (imgui::Button("选择文件夹")) {
    open_file_dialog{"comm_create_video",
                     "select dir",
                     nullptr,
                     ".",
                     "",
                     0}
        .show(
            [this](const std::vector<FSys::path>& in) {
              boost::copy(in | boost::adaptors::transformed(
                                   [](const FSys::path& in_path) {
                                     image_paths k_image_paths{};
                                     k_image_paths.use_dir = true;
                                     k_image_paths.p_path_list.emplace_back(in_path);
                                     k_image_paths.p_show_name = fmt::format("{}##{}",
                                                                             k_image_paths.p_path_list.back().generic_string(),
                                                                             fmt::ptr(&k_image_paths));
                                     return k_image_paths;
                                   }),
                          std::back_inserter(p_image_path));
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
      image_sequence_ptr ptr{};
      if (i.use_dir) {
        ptr = image->set_path(i.p_path_list.front());
      } else {
        ptr = image->set_path(i.p_path_list);
      }
      ptr->set_out_dir(*p_out_path);
      ptr->set_shot_and_eps(shot::analysis_static(i.p_path_list.front()),
                            episodes::analysis_static(i.p_path_list.front()));

      image->create_video(*p_out_path);
    }
  }

  dear::ListBox{"image_list"} && [this]() {
    for (const auto& i : p_image_path) {
      dear::Selectable(i.p_show_name);
    }
  };

  if (imgui::Button("选择视频")) {
    open_file_dialog{"comm_create_video",
                     "select dir",
                     ".mp4",
                     ".",
                     "",
                     0}
        .show(
            [this](const std::vector<FSys::path>& in) {
              p_video_path = in;
            });
  }
  imgui::SameLine();
  if (imgui::Button("连接视频")) {
    auto video  = new_object<video_sequence_async>();
    auto k_v    = video->set_video_list(p_video_path);
    auto k_name = k_v->set_shot_and_eps(shot::analysis_static(p_video_path.front()),
                                        episodes::analysis_static(p_video_path.front()));
    video->connect_video(k_name.empty() ? FSys::path{} : FSys::path{*p_out_path} / k_name);
  }

  dear::ListBox{"video_list"} && [this]() {
    for (const auto& i : p_video_path) {
      dear::Selectable(i.filename().generic_string());
    }
  };

  return true;
}
comm_import_ue_files::comm_import_ue_files()
    : p_ue4_prj(),
      p_ue4_show(new_object<std::string>()) {
  p_name = "ue工具";
}
bool comm_import_ue_files::is_async() {
  return true;
}
bool comm_import_ue_files::render() {
  imgui::InputText("ue项目", p_ue4_show.get());
  imgui::SameLine();
  if (imgui::Button("选择")) {
    open_file_dialog{
        "comm_create_video",
        "选择",
        ".uproject",
        ".",
        "",
        1}
        .show(
            [this](const std::vector<FSys::path>& in_p) {
              if (!in_p.empty()) {
                *p_ue4_show = in_p.front().generic_string();
                p_ue4_prj   = in_p.front();
              }
            });
  }
  imgui::SameLine();
  if (imgui::Button("选择导入")) {
    open_file_dialog{
        "comm_create_video",
        "选择",
        "files (*.abc *.fbx){.fbx,.abc}",
        ".",
        "",
        0}
        .show(
            [this](const std::vector<FSys::path>& in_p) {
              p_import_list = in_p;
            });
  }
  if (imgui::Button("导入")) {
    auto ue = new_object<ue4_project_async>();
    ue->set_ue4_project(p_ue4_prj);
    for (const auto& i : p_import_list) {
      ue->import_file(i);
    }
  }
  dear::ListBox{"文件列表"} && [this]() {
    for (const auto& in : p_import_list)
      dear::Selectable(in.filename().generic_string());
  };

  return command_base::add_data();
}

}  // namespace doodle
