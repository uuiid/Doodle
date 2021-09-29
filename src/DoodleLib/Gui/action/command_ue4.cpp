//
// Created by TD on 2021/9/18.
//

#include "command_ue4.h"

#include <DoodleLib/FileWarp/ue4_project.h>
#include <DoodleLib/Gui/widget_register.h>
#include <DoodleLib/Gui/widgets/assets_widget.h>
#include <DoodleLib/Gui/widgets/time_widget.h>
#include <DoodleLib/Metadata/metadata_cpp.h>
#include <DoodleLib/core/doodle_lib.h>
#include <DoodleLib/core/open_file_dialog.h>
#include <DoodleLib/doodle_app.h>
#include <DoodleLib/libWarp/imgui_warp.h>
namespace doodle {
comm_ass_ue4_create_shot::comm_ass_ue4_create_shot()
    : p_ue4(new_object<ue4_project_async>()),
      p_ue4_prj_path(new_object<string>()),
      p_parent() {
  p_name     = "创建ue4镜头";
  p_show_str = make_imgui_name(this,
                               "创建镜头序列",
                               "ue路径",
                               "选择");
}

bool comm_ass_ue4_create_shot::render() {
  if (p_parent) {
    imgui::InputText(p_show_str["ue路径"].c_str(), p_ue4_prj_path.get());
    if (imgui::Button(p_show_str["选择"].c_str()))
      open_file_dialog{"open_ue4_path",
                       "打开文件路径",
                       ".uproject",
                       ".",
                       "",
                       1}
          .show([this](const std::vector<FSys::path>& in_path) {
            *p_ue4_prj_path = in_path.front().generic_string();
          });

    if (imgui::Button(p_show_str["创建镜头序列"].c_str())) {
      auto k_ass = doodle_app::Get()->get_register()->get_widget<assets_widget>();
      if (k_ass) {
        auto k_all = k_ass->p_all_selected;
        std::vector<shot_ptr> k_list;
        boost::copy(
            k_all |
                boost::adaptors::filtered([](auto in) {
                  return details::is_class<shot>(in);
                }) |
                boost::adaptors::transformed([](auto in) {
                  return std::dynamic_pointer_cast<shot>(in);
                }),
            std::back_inserter(k_list));
        p_ue4->create_shot_folder(k_list);
      }
      // p_ue4->
    }
  }
  return true;
}

bool comm_ass_ue4_create_shot::add_data(const metadata_ptr& in_parent, const metadata_ptr& in) {
  p_parent = in_parent;
  return true;
}

comm_ass_ue4_import::comm_ass_ue4_import() {
}

bool comm_ass_ue4_import::render() {
  return true;
}

bool comm_ass_ue4_import::add_data(const metadata_ptr& in_parent, const metadata_ptr& in) {
  return true;
}
}  // namespace doodle
