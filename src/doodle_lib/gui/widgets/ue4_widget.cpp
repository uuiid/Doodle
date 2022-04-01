//
// Created by TD on 2022/4/1.
//

#include "ue4_widget.h"
#include <doodle_lib/gui/gui_ref/ref_base.h>
namespace doodle {
class ue4_widget::impl {
 public:
  impl()             = default;
  using ue4_file_gui = gui::gui_cache<std::string>;

  gui::gui_cache<std::string, gui::gui_cache_path> ue4_prj{"ue4项目"s, FSys::path{}};
  std::vector<ue4_file_gui> import_list_files;

  gui::gui_cache<bool> import_cam{"导入cam"s, true};
  gui::gui_cache<bool> import_abc{"导入abc"s, true};
  gui::gui_cache<bool> import_fbx{"导入fbx"s, true};
  gui::gui_cache<bool> quit_{"生成并退出"s, true};
  gui::gui_cache_name_id import_{"导入"};
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
    const std::chrono::duration<std::chrono::system_clock::rep,
                                std::chrono::system_clock::period> &,
    void *data) {

}
}  // namespace doodle
