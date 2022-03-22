//
// Created by TD on 2021/9/18.
//

#include "command_ue4.h"

#include <doodle_lib/core/doodle_lib.h>
#include <gui/open_file_dialog.h>
#include <doodle_lib/file_warp/ue4_project.h>
#include <doodle_lib/gui/widget_register.h>
#include <doodle_lib/gui/widgets/assets_filter_widget.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata_cpp.h>
namespace doodle {
comm_ass_ue4_create_shot::comm_ass_ue4_create_shot()
    : p_ue4_prj_path() {
  p_name     = "创建ue4镜头";
  p_show_str = {{"创建镜头序列", "创建镜头序列"},
                {"ue路径", "ue路径"},
                {"选择", "选择"},
                {"获得选择", "获得选择"},
                {"shot列表", "shot列表"}};
}

bool comm_ass_ue4_create_shot::render() {
  imgui::InputText(p_show_str["ue路径"].c_str(), &p_ue4_prj_path);
  imgui::SameLine();
  if (imgui::Button(p_show_str["选择"].c_str())) {
    auto l_ptr = std::make_shared<std::vector<FSys::path>>();
    g_main_loop().attach<file_dialog>(file_dialog::dialog_args{l_ptr}
                                          .set_title("打开文件路径"s)
                                          .add_filter(".uproject"s))
        .then<one_process_t>(
            [this, l_ptr]() {
              p_ue4_prj_path = l_ptr->front().generic_string();
            });
  }
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

comm_ass_ue4_import::comm_ass_ue4_import() = default;

bool comm_ass_ue4_import::render() {
  return true;
}

}  // namespace doodle
