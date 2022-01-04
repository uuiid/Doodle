//
// Created by TD on 2021/9/22.
//

#include "open_file_dialog.h"

#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/doodle_app.h>
#include <doodle_lib/lib_warp/imgui_warp.h>

#include <imfilebrowser.h>
namespace doodle {

void open_file_dialog::show(const std::function<void(const std::vector<FSys::path> &)> &in_fun) {
  doodle_app::Get()->main_loop.connect_extended(
      [kes = this->p_vKey, fun = in_fun](const doodle_app::connection &in) {
        dear::OpenFileDialog{kes} && [k_fun = fun, &in]() {
          auto ig = ImGuiFileDialog::Instance();
          if (ig->IsOk()) {
            auto filter             = ig->GetCurrentFilter();
            FSys::path k_get_curr_p = ig->GetFilePathName();
            auto k_get_curr         = k_get_curr_p.parent_path();
            std::vector<FSys::path> k_list{};
            auto selected = ig->GetSelection();
            std::transform(selected.begin(), selected.end(),
                           std::back_inserter(k_list),
                           [&k_get_curr](const auto &k) {
                             return k_get_curr / k.first;
                           });
            if (selected.empty())
              k_list.emplace_back(k_get_curr);
            k_fun(k_list);
          }
          in.disconnect();
        };
      });
};

class file_dialog::impl {
 public:
  explicit impl(::ImGuiFileBrowserFlags in_flags) : p_file_dialog(in_flags){};
  imgui::FileBrowser p_file_dialog;
  imgui::FileDialog p_dialog{};
  select_sig p_sig;
};
file_dialog::file_dialog(const file_dialog::select_sig &in_sig,
                         const string &in_title,
                         std::int32_t in_flags,
                         const std::vector<string> &in_filters,
                         const FSys::path &in_pwd)
    : p_i(std::make_unique<impl>(in_flags)) {
  p_i->p_sig = in_sig;
  p_i->p_file_dialog.SetTitle(in_title);
  p_i->p_file_dialog.SetTypeFilters(in_filters);
  p_i->p_file_dialog.SetPwd(in_pwd);
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
}
void file_dialog::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void *data) {
  p_i->p_file_dialog.Display();
  if (p_i->p_file_dialog.HasSelected()) {
    std::visit(entt::overloaded{
                   [&](const one_sig &in_sig) -> void {
                     in_sig(p_i->p_file_dialog.GetSelected());
                     this->succeed();
                   },
                   [&](const mult_sig &in_sig) -> void {
                     in_sig(p_i->p_file_dialog.GetMultiSelected());
                     this->succeed();
                   }},
               p_i->p_sig);

  }
}
void file_dialog::succeeded() {
  p_i->p_dialog.Close();
}
void file_dialog::failed() {
  p_i->p_dialog.Close();
}
void file_dialog::aborted() {
  p_i->p_dialog.Close();
}
}  // namespace doodle
