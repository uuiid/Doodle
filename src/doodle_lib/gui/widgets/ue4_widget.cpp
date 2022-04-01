//
// Created by TD on 2022/4/1.
//

#include "ue4_widget.h"
#include <doodle_lib/gui/gui_ref/ref_base.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/long_task/process_pool.h>
#include <doodle_lib/gui/open_file_dialog.h>

namespace doodle {
class ue4_widget::impl {
  class file_data {
   public:
    file_data() = default;
    entt::handle handle_;
  };

 public:
  impl()             = default;
  using ue4_file_gui = gui::gui_cache<std::string, file_data>;

  gui::gui_cache<std::string, gui::gui_cache_path> ue4_prj{"ue4项目"s, ""s};
  std::vector<ue4_file_gui> import_list_files;

  gui::gui_cache<bool> import_cam{"导入cam"s, true};
  gui::gui_cache<bool> import_abc{"导入abc"s, true};
  gui::gui_cache<bool> import_fbx{"导入fbx"s, true};
  gui::gui_cache<bool> quit_{"生成并退出"s, true};
  gui::gui_cache_name_id import_{"导入"s};
  gui::gui_cache_name_id open_file_dig{"选择"s};
};

ue4_widget::ue4_widget()
    : p_i(std::make_unique<impl>()) {
}
ue4_widget::~ue4_widget() = default;

void ue4_widget::init() {
  g_reg()->set<ue4_widget &>(*this);
}

void ue4_widget::succeeded() {
  g_reg()->unset<ue4_widget>();
}

void ue4_widget::failed() {
  g_reg()->unset<ue4_widget>();
}

void ue4_widget::aborted() {
  g_reg()->unset<ue4_widget>();
}

void ue4_widget::update(
    const std::chrono::duration<
        std::chrono::system_clock::rep,
        std::chrono::system_clock::period> &,
    void *data) {
  if (ImGui::InputText(*p_i->ue4_prj.gui_name, &p_i->ue4_prj.data))
    p_i->ue4_prj.path = p_i->ue4_prj.data;
  ImGui::SameLine();
  if (ImGui::Button(*p_i->open_file_dig)) {
    auto l_p = std::make_shared<FSys::path>();
    g_main_loop().attach<file_dialog>(
                     file_dialog::dialog_args{l_p}
                         .set_use_dir()
                         .add_filter(".uproject"s))
        .then<one_process_t>([this, l_p]() {
          this->p_i->ue4_prj.data = l_p->generic_string();
          this->p_i->ue4_prj.path = *l_p;
        });
  }
}
}  // namespace doodle
