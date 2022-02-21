//
// Created by TD on 2021/9/18.
//

#include "command_tool.h"

#include <gui/open_file_dialog.h>
#include <doodle_lib/file_warp/image_sequence.h>
#include <doodle_lib/file_warp/maya_file.h>
#include <doodle_lib/file_warp/ue4_project.h>
#include <doodle_lib/file_warp/video_sequence.h>
#include <doodle_lib/metadata/episodes.h>
#include <doodle_lib/metadata/project.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
namespace doodle {

comm_maya_tool::comm_maya_tool()
    : p_cloth_path(),
      p_text(),
      p_sim_path(),
      p_only_sim(false),
      p_use_all_ref(false) {
}
void comm_maya_tool::init() {
  auto k_prj = g_reg()->try_ctx<root_ref>();
  chick_true<doodle_error>(k_prj, DOODLE_LOC, "没有项目选中");

  p_text = k_prj->root_handle().get<project>().get_vfx_cloth_config().vfx_cloth_sim_path.generic_string();
  g_reg()->set<comm_maya_tool&>(*this);
}
void comm_maya_tool::succeeded() {
}
void comm_maya_tool::failed() {
}
void comm_maya_tool::aborted() {
}
void comm_maya_tool::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
  this->render();
}
void comm_maya_tool::render() {
  if (imgui::Button("maya文件")) {
    p_sim_path.clear();
    g_main_loop().attach<file_dialog>(
        [this](const std::vector<FSys::path>& in_p) {
          p_sim_path = in_p;
        },
        "select_maya_file",
        std::vector<string>{".ma", ".mb"});
  }

  dear::ListBox{"file_list"} && [this]() {
    for (const auto& f : p_sim_path) {
      dear::Selectable(f.generic_string());
    }
  };

  dear::Text(fmt::format("解算资产: {}", p_text));

  dear::TreeNode{"解算设置"} && [this]() {
    imgui::Checkbox("只解算不替换引用", &p_only_sim);
  };
  dear::TreeNode{"fbx导出设置"} && [&]() {
    imgui::Checkbox("直接加载所有引用", &p_use_all_ref);
  };

  if (imgui::Button("解算")) {
    auto maya = new_object<maya_file_async>();
    std::for_each(p_sim_path.begin(), p_sim_path.end(),
                  [this, maya](const FSys::path& in_path) {
                    auto arg               = details::qcloth_arg{};
                    arg.sim_path           = in_path;
                    arg.qcloth_assets_path = p_cloth_path;
                    arg.only_sim           = p_only_sim;
                    arg.project_           = g_reg()->ctx<database_info>().path_;
                    maya->qcloth_sim_file(make_handle(), arg);
                  });
  }
  if (imgui::Button("fbx导出")) {
    auto maya = new_object<maya_file_async>();
    std::for_each(p_sim_path.begin(), p_sim_path.end(),
                  [maya, this](const auto& i) {
                    auto k_arg        = details::export_fbx_arg{};
                    k_arg.file_path   = i;
                    k_arg.use_all_ref = this->p_use_all_ref;
                    k_arg.project_    = g_reg()->ctx<database_info>().path_;
                    maya->export_fbx_file(make_handle(), k_arg);
                  });
  }
}

comm_create_video::comm_create_video()
    : p_video_path(),
      p_image_path(),
      p_out_path(new_object<std::string>()) {
}
void comm_create_video::init() {
  g_reg()->set<comm_create_video&>(*this);
}
void comm_create_video::succeeded() {
}
void comm_create_video::failed() {
}
void comm_create_video::aborted() {
}
void comm_create_video::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
  this->render();
}
void comm_create_video::render() {
  imgui::InputText("输出文件夹", p_out_path.get());
  imgui::SameLine();
  if (imgui::Button("选择")) {
    g_main_loop().attach<file_dialog>(
        [this](const FSys::path& in_p) {
          *p_out_path = in_p.generic_string();
        },
        "选择目录");
  }

  if (imgui::Button("选择图片")) {
    g_main_loop().attach<file_dialog>(
        [this](const std::vector<FSys::path>& in) {
          image_paths k_image_paths{};
          k_image_paths.use_dir     = false;
          k_image_paths.p_path_list = in;
          k_image_paths.p_show_name = k_image_paths.p_path_list.front().parent_path().generic_string();
          p_image_path.emplace_back(std::move(k_image_paths));
        },
        "选择序列",
        string_list{".png", ".jpg"});
  }
  imgui::SameLine();
  if (imgui::Button("选择文件夹")) {
    g_main_loop().attach<file_dialog>(
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
        },
        "select dir");
  }

  imgui::SameLine();
  if (imgui::Button("清除")) {
    p_image_path.clear();
  }
  imgui::SameLine();
  if (imgui::Button("创建视频")) {
  }

  dear::ListBox{"image_list"} && [this]() {
    for (const auto& i : p_image_path) {
      dear::Selectable(i.p_show_name);
    }
  };

  if (imgui::Button("选择视频")) {
    g_main_loop().attach<file_dialog>(
        [this](const std::vector<FSys::path>& in) {
          p_video_path = in;
        },
        "select mp4 file",
        string_list{".mp4"});
  }
  imgui::SameLine();
  if (imgui::Button("连接视频")) {
  }

  dear::ListBox{"video_list"} && [this]() {
    for (const auto& i : p_video_path) {
      dear::Selectable(i.filename().generic_string());
    }
  };
}

comm_import_ue_files::comm_import_ue_files()
    : p_ue4_prj(),
      p_ue4_show(new_object<std::string>()) {
}
void comm_import_ue_files::init() {
  g_reg()->set<comm_import_ue_files&>(*this);
}
void comm_import_ue_files::succeeded() {
}
void comm_import_ue_files::failed() {
}
void comm_import_ue_files::aborted() {
}
void comm_import_ue_files::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
  this->render();
}
void comm_import_ue_files::render() {
  imgui::InputText("ue项目", p_ue4_show.get());
  imgui::SameLine();
  if (imgui::Button("选择")) {
    g_main_loop().attach<file_dialog>(
        [this](const FSys::path& in_p) {
          if (!in_p.empty()) {
            *p_ue4_show = in_p.generic_string();
            p_ue4_prj   = in_p;
          }
        },
        "select",
        string_list{".uproject"});
  }
  imgui::SameLine();
  if (imgui::Button("选择导入")) {
    g_main_loop().attach<file_dialog>(
        [this](const std::vector<FSys::path>& in_p) {
          p_import_list = in_p;
        },
        "select",
        string_list{".fbx", ".abc"});
  }
  if (imgui::Button("导入")) {
  }
  dear::ListBox{"文件列表"} && [this]() {
    for (const auto& in : p_import_list)
      dear::Selectable(in.filename().generic_string());
  };
}

}  // namespace doodle
