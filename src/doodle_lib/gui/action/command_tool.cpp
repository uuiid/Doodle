//
// Created by TD on 2021/9/18.
//

#include "command_tool.h"

#include <doodle_lib/core/open_file_dialog.h>
#include <doodle_lib/file_warp/image_sequence.h>
#include <doodle_lib/file_warp/maya_file.h>
#include <doodle_lib/file_warp/ue4_project.h>
#include <doodle_lib/file_warp/video_sequence.h>
#include <doodle_lib/metadata/episodes.h>
#include <doodle_lib/metadata/project.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/core/core_set.h>

namespace doodle {

comm_maya_tool::comm_maya_tool()
    : p_cloth_path(),
      p_text(),
      p_sim_path(),
      p_only_sim(false),
      p_max_th(core_set::getSet().p_max_thread),
      p_use_all_ref(false) {
  p_name     = "maya工具";
  auto k_prj = g_reg()->try_ctx<root_ref>();
  chick_true<doodle_error>(k_prj, DOODLE_LOC, "没有项目选中");

  p_text = k_prj->root_handle().get<project>().get_vfx_cloth_config().vfx_cloth_sim_path.generic_string();
}
bool comm_maya_tool::is_async() {
  return true;
}
bool comm_maya_tool::render() {
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

  dear::Text(fmt::format("解算资产: {}", p_text));

  dear::TreeNode{"通用设置"} && [this]() {
    imgui::SliderInt("最大任务数", &(p_max_th), 0u, 64u);
  };

  dear::TreeNode{"解算设置"} && [this]() {
    imgui::Checkbox("只解算不替换引用", &p_only_sim);
  };
  dear::TreeNode{"fbx导出设置"} && [&]() {
    imgui::Checkbox("直接加载所有引用", &p_use_all_ref);
  };

  if (imgui::Button("解算")) {
    maya_file_async l_maya_file_async{};
    auto arg               = details::qcloth_arg{};
    arg.qcloth_assets_path = p_cloth_path;
    arg.only_sim           = p_only_sim;
    l_maya_file_async.qcloth_sim_file(p_sim_path, arg, p_max_th);
  }
  if (imgui::Button("fbx导出")) {
    maya_file_async l_maya_file_async{};
    auto k_arg        = details::export_fbx_arg{};
    k_arg.use_all_ref = this->p_use_all_ref;
    l_maya_file_async.export_fbx_file(p_sim_path, k_arg, p_max_th);
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
      ptr->set_out_path(*p_out_path);
      auto l_w = details::watermark{};
      l_w.path_to_ep_sc(i.p_path_list.front());
      ptr->add_watermark(l_w);
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
    auto k_name = k_v->set_shot_and_eps(p_video_path.front());
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

  return true;
}

}  // namespace doodle
