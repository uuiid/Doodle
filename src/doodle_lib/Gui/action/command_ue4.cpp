//
// Created by TD on 2021/9/18.
//

#include "command_ue4.h"

#include <doodle_lib/file_warp/ue4_project.h>
#include <doodle_lib/Gui/widget_register.h>
#include <doodle_lib/Gui/widgets/assets_widget.h>
#include <doodle_lib/Gui/widgets/time_widget.h>
#include <doodle_lib/Metadata/metadata_cpp.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/core/open_file_dialog.h>
#include <doodle_lib/doodle_app.h>
#include <doodle_lib/libWarp/imgui_warp.h>
namespace doodle {
comm_ass_ue4_create_shot::comm_ass_ue4_create_shot()
    : p_ue4(new_object<ue4_project_async>()),
      p_ue4_prj_path(new_object<string>()),
      p_parent() {
  p_name     = "创建ue4镜头";
  p_show_str = make_imgui_name(this,
                               "创建镜头序列",
                               "ue路径",
                               "选择",
                               "获得选择",
                               "shot列表");
}

bool comm_ass_ue4_create_shot::render() {
  if (p_parent) {
    imgui::InputText(p_show_str["ue路径"].c_str(), p_ue4_prj_path.get());
    imgui::SameLine();
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
    if (imgui::Button(p_show_str["获得选择"].c_str())) {
      auto k_ass = doodle_app::Get()->get_register()->get_widget<assets_widget>();
      if (k_ass) {
        auto k_all = k_ass->p_all_selected;
        p_shot_list.clear();
        boost::copy(
            k_all |
                boost::adaptors::filtered([](auto in) {
                  return details::is_class<shot>(in);
                }) |
                boost::adaptors::transformed([](auto in) {
                  return std::dynamic_pointer_cast<shot>(in);
                }),
            std::back_inserter(p_shot_list));
        boost::sort(p_shot_list, boost::less_pointees_t<shot_ptr>{});
      }
    }
    imgui::SameLine();
    if (imgui::Button(p_show_str["创建镜头序列"].c_str())) {
      if (!p_shot_list.empty()) {
        p_ue4->set_ue4_project(*p_ue4_prj_path);
        p_ue4->create_shot_folder(p_shot_list);
      }
    }

    dear::ListBox{p_show_str["shot列表"].c_str()} && [this]() {
      for (const auto& s : p_shot_list) {
        dear::Selectable(s->show_str());
      }
    };
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
