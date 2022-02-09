//
// Created by TD on 2021/9/18.
//

#include "command_ue4.h"

#include <doodle_lib/core/doodle_lib.h>
#include <gui/open_file_dialog.h>
#include <doodle_lib/file_warp/ue4_project.h>
#include <doodle_lib/gui/widget_register.h>
#include <doodle_lib/gui/widgets/assets_widget.h>
#include <doodle_lib/gui/widgets/time_widget.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata_cpp.h>
namespace doodle {
comm_ass_ue4_create_shot::comm_ass_ue4_create_shot()
    : p_ue4_prj_path(new_object<string>()) {
  p_name     = "创建ue4镜头";
  p_show_str = make_imgui_name(this,
                               "创建镜头序列",
                               "ue路径",
                               "选择",
                               "获得选择",
                               "shot列表");
}

bool comm_ass_ue4_create_shot::render() {
  imgui::InputText(p_show_str["ue路径"].c_str(), p_ue4_prj_path.get());
  imgui::SameLine();
  if (imgui::Button(p_show_str["选择"].c_str()))
    g_main_loop().attach<file_dialog>(
        [this](const std::vector<FSys::path>& in_path) {
          *p_ue4_prj_path = in_path.front().generic_string();
        },
        "打开文件路径",
        string_list{".uproject"});
  if (imgui::Button(p_show_str["获得选择"].c_str())) {
  }
  imgui::SameLine();
  if (imgui::Button(p_show_str["创建镜头序列"].c_str())) {
  }

  dear::ListBox{p_show_str["shot列表"].c_str()} && [this]() {
    for (const auto& s : p_shot_list) {
      dear::Selectable(s.get<shot>().str());
    }
  };

  return true;
}

comm_ass_ue4_import::comm_ass_ue4_import() {
}

bool comm_ass_ue4_import::render() {
  return true;
}

}  // namespace doodle
