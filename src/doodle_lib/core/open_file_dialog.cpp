//
// Created by TD on 2021/9/22.
//

#include "open_file_dialog.h"

#include <doodle_lib/lib_warp/imgui_warp.h>

#include <gui/widgets/file_browser.h>
namespace doodle {

;

class file_dialog::impl {
 public:
//  explicit impl() : p_file_dialog(in_flags){};
  file_browser p_file_dialog;
  select_sig p_sig;
};
file_dialog::file_dialog(const file_dialog::select_sig &in_sig,
                         const string &in_title,
                         std::int32_t in_flags,
                         const std::vector<string> &in_filters,
                         const FSys::path &in_pwd)
    : p_i(std::make_unique<impl>()) {
  p_i->p_sig = in_sig;
//  p_i->p_file_dialog.(in_title);
  p_i->p_file_dialog.set_filter(in_filters);
  p_i->p_file_dialog.set_pwd_path(in_pwd);
}
file_dialog::file_dialog(const file_dialog::select_sig &in_function,
                         const string &in_title)
    : file_dialog(in_function,
                  in_title,
                  (in_function.index() == 1
                       ? flags::ImGuiFileBrowserFlags_MultipleSelection
                       : flags::ImGuiFileBrowserFlags_SelectDirectory) |
                      flags::ImGuiFileBrowserFlags_SelectDirectory,
                  {},
                  FSys::current_path()) {
}
file_dialog::file_dialog(const file_dialog::select_sig &in_function,
                         const string &in_title,
                         const std::vector<string> &in_filters,
                         const FSys::path &in_pwd)
    : file_dialog(in_function,
                  in_title,
                  (in_function.index() == 1
                       ? flags::ImGuiFileBrowserFlags_MultipleSelection
                       : 0),
                  in_filters,
                  in_pwd) {
}
file_dialog::~file_dialog() = default;

void file_dialog::init() {
  p_i->p_file_dialog.open();
}
void file_dialog::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void *data) {
  p_i->p_file_dialog.render();
  if (p_i->p_file_dialog.is_ok()) {
    std::visit(entt::overloaded{
                   [&](const one_sig &in_sig) -> void {
                     in_sig(p_i->p_file_dialog.get_select());
                     this->succeed();
                   },
                   [&](const mult_sig &in_sig) -> void {
                     in_sig(p_i->p_file_dialog.get_selects());
                     this->succeed();
                   }},
               p_i->p_sig);
  }else{
    this->fail();
  }

}
void file_dialog::succeeded() {
  p_i->p_file_dialog.close();
}
void file_dialog::failed() {
  p_i->p_file_dialog.close();
}
void file_dialog::aborted() {
  p_i->p_file_dialog.close();
}
}  // namespace doodle
