//
// Created by TD on 2021/9/18.
//

#include "command_files.h"

#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/core/open_file_dialog.h>
#include <doodle_lib/metadata/metadata_cpp.h>
#include <doodle_lib/rpc/rpc_file_system_client.h>
#include <doodle_lib/rpc/rpc_trans_path.h>
namespace doodle {
comm_files_up::comm_files_up()
    : p_root(),
      p_use_relative(new_object<bool>(false)),
      p_file() {
  p_name     = "添加文件";
  p_show_str = make_imgui_name(this,
                               "添加文件",
                               "添加",
                               "替换",
                               "相对路径",
                               "路径列表");
}

bool comm_files_up::render() {
  if (p_root) {
    if (imgui::Button(p_show_str["添加文件"].c_str())) {
      open_file_dialog{
          "open_get_files",
          "获得文件",
          ".*",
          ".",
          "",
          1}
          .show(
              [this](const std::vector<FSys::path>& in_p) {
                p_file      = in_p.front();
                p_list_path = p_root->get_path_file()->add_file(p_file, *p_use_relative);
              });
    }
    imgui::SameLine();
    if (imgui::Checkbox(p_show_str["相对路径"].c_str(), p_use_relative.get())) {
      p_list_path = p_root->get_path_file()->add_file(p_file, *p_use_relative);
    }

    if (imgui::Button(p_show_str["添加"].c_str())) {
      if (!p_list_path.empty()) {
        add_files();
      }
    }
    imgui::SameLine();
    if (imgui::Button(p_show_str["替换"].c_str())) {
      if (!p_list_path.empty()) {
        p_root->get_path_file()->get().clear();
        add_files();
      }
    }
    dear::ListBox{
        p_show_str["路径列表"].c_str(),
        ImVec2{-FLT_MIN, 5 * imgui::GetTextLineHeightWithSpacing()}} &&
        [this]() {
          for (auto& i : p_list_path) {
            auto str = fmt::format("本地 {} \n服务器 {}", i->get_local_path(),
                                   i->get_server_path());
            imgui::Selectable(str.c_str());
          }
        };
  }
  return true;
}

bool comm_files_up::set_child() {
  if (std::holds_alternative<assets_file_ptr>(p_var)) {
    p_root = std::get<assets_file_ptr>(p_var);
    if (p_root && !p_root->get_path_file()) {
      p_root->set_path_file(new_object<assets_path_vector>());
    }
  }
  return true;
}
bool comm_files_up::add_files() {
  rpc_trans_path_ptr_list k_list{};
  for (auto& i : p_list_path) {
    k_list.emplace_back(std::make_unique<rpc_trans_path>(i));
    p_root->get_path_file()->get().push_back(i);
  }
  p_root->up_version();
  p_root->updata_db();
  auto k_up = doodle_lib::Get().get_rpc_file_system_client()->upload(k_list);
  (*k_up)();
  return false;
}
}  // namespace doodle
