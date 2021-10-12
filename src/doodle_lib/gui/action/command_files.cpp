//
// Created by TD on 2021/9/18.
//

#include "command_files.h"

#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/core/open_file_dialog.h>
#include <doodle_lib/file_warp/image_sequence.h>
#include <doodle_lib/metadata/metadata_cpp.h>
#include <doodle_lib/rpc/rpc_file_system_client.h>
#include <doodle_lib/rpc/rpc_trans_path.h>

namespace doodle {
comm_files_select::comm_files_select()
    : p_root(),
      p_use_relative(new_object<bool>(false)),
      p_file(),
      p_list_paths(new_object<assets_path_vector>()) {
  p_name     = "添加文件";
  p_show_str = make_imgui_name(this,
                               "添加文件",
                               "添加",
                               "替换",
                               "相对路径",
                               "路径列表");
}

bool comm_files_select::render() {
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
                p_file = in_p.front();
                p_list_paths->get().clear();
                p_comm_sub = p_list_paths->add_file(p_file, *p_use_relative);
                p_comm_sub->add_data(p_root, p_list_paths);
              });
    }
    imgui::SameLine();
    if (imgui::Checkbox(p_show_str["相对路径"].c_str(), p_use_relative.get())) {
      p_list_paths->get().clear();
      p_comm_sub = p_list_paths->add_file(p_file, *p_use_relative);
      p_comm_sub->add_data(p_root, p_list_paths);
    }

    dear::ListBox{
        p_show_str["路径列表"].c_str(),
        ImVec2{-FLT_MIN, 5 * imgui::GetTextLineHeightWithSpacing()}} &&
        [this]() {
          for (auto& i : p_list_paths->get()) {
            auto str = fmt::format("本地 {} \n服务器 {}", i->get_local_path(),
                                   i->get_server_path());
            imgui::Selectable(str.c_str());
          }
        };
    if (p_comm_sub) {
      p_comm_sub->render();
    }
    if (imgui::Button(p_show_str["添加"].c_str())) {
      if (!p_list_paths->get().empty()) {
        add_files();
      }
    }
    imgui::SameLine();
    if (imgui::Button(p_show_str["替换"].c_str())) {
      if (!p_list_paths->get().empty()) {
        p_root->get_path_file()->get().clear();
        add_files();
      }
    }
  }
  return true;
}

bool comm_files_select::set_child() {
  if (std::holds_alternative<assets_file_ptr>(p_var)) {
    p_root = std::get<assets_file_ptr>(p_var);
    p_list_paths->set_metadata(p_root);
    if (p_root && !p_root->get_path_file()) {
      p_root->set_path_file(new_object<assets_path_vector>());
    }
  }
  return true;
}
bool comm_files_select::add_files() {
  p_root->get_path_file()->merge(*p_list_paths);
  p_root->up_version();
  p_root->updata_db();
  auto k_up = doodle_lib::Get().get_rpc_file_system_client()->upload(p_list_paths->make_up_path());
  (*k_up)();
  return false;
}

void comm_file_image_to_move::init() {
  p_name         = "视频选项";
  p_show_str     = make_imgui_name(this, "不上传", "上传视频", "不上传源文件");
  p_image_create = new_object<image_sequence_async>();
}

bool comm_file_image_to_move::set_child() {
  if (std::holds_alternative<assets_path_vector_ptr>(p_var)) {
    p_list_paths = std::get<assets_path_vector_ptr>(p_var);
    p_image      = p_image_create->set_path(p_list_paths);
    p_out_file   = p_image->get_out_path();
    p_text       = fmt::format("本地生成路径 {}",p_out_file);
  }
  return false;
}

bool comm_file_image_to_move::render() {
  if (p_list_paths) {
    imgui::Checkbox(p_show_str["不上传"].c_str(), p_not_up_file.get());
    imgui::SameLine();
    imgui::Checkbox(p_show_str["不上传源文件"].c_str(), p_not_up_file.get());

    imgui::Text(p_text.c_str());

    if (imgui::Button(p_show_str["上传视频"].c_str())) {
      p_image_create->create_video();
    }
  }
  return false;
}
}  // namespace doodle
