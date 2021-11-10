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
      p_file(){
  p_name     = "添加文件";
  p_show_str = make_imgui_name(this,
                               "添加文件",
                               "添加",
                               "替换",
                               "相对路径",
                               "路径列表",
                               "获得文件");
}

bool comm_files_select::render() {
  if (p_root) {
    if (imgui::Button(p_show_str["添加文件"].c_str())) {
      open_file_dialog{
          p_show_str["获得文件"].c_str(),
          "获得文件",
          ".*",
          ".",
          "",
          1}
          .show(
              [this](const std::vector<FSys::path>& in_p) {
                p_file = in_p.front();
                p_root.get<assets_path_vector>().get().clear();
                p_comm_sub = p_root.get<assets_path_vector>().add_file(p_file, *p_use_relative);
                if (p_comm_sub) {
                  p_comm_sub->set_data(p_root);
                }
              });
    }
    imgui::SameLine();
    if (imgui::Checkbox(p_show_str["相对路径"].c_str(), p_use_relative.get())) {
      p_root.get<assets_path_vector>().get().clear();
      p_comm_sub = p_root.get<assets_path_vector>().add_file(p_file, *p_use_relative);
      if (p_comm_sub) {
        p_comm_sub->set_data(p_root);
      }
    }

    dear::ListBox{
        p_show_str["路径列表"].c_str(),
        ImVec2{-FLT_MIN, 5 * imgui::GetTextLineHeightWithSpacing()}} &&
        [this]() {
          for (auto& i : p_root.get_or_emplace<assets_path_vector>().get()) {
            auto str = fmt::format("本地 {} \n服务器 {}", i.get_local_path(),
                                   i.get_server_path());
            imgui::Selectable(str.c_str());
          }
        };
    if (p_comm_sub) {
      p_comm_sub->render();
    }
  }
  return true;
}

bool comm_files_select::set_data(const entt::handle& in_data) {
  if (in_data.any_of<assets_file>()) {
    p_root = in_data;
  }
  return true;
}

comm_files_up::comm_files_up()
    : command_base(),
      p_list_paths() {
  p_show_str = make_imgui_name(this, "添加");
}

bool comm_files_up::add_files() {
  p_list_paths.get<assets_file>().up_version();
  auto k_up = doodle_lib::Get().get_rpc_file_system_client()->upload(
      p_list_paths.get<assets_path_vector>().make_up_path());
  (*k_up)();
  p_list_paths.patch<database_stauts>(database_set_stauts<need_save>{});
  return false;
}

bool comm_files_up::render() {
  if (p_list_paths) {
    if (imgui::Button(p_show_str["添加"].c_str())) {
      if (!p_list_paths.get<assets_path_vector>().get().empty()) {
        add_files();
      }
    }
  }
  return true;
}

bool comm_files_up::set_data(const entt::handle& in_data) {
  if (in_data.any_of<assets_path_vector>()) {
    p_list_paths = in_data;
  } else {
    p_list_paths = entt::handle{};
  }
  return true;
}

void comm_file_image_to_move::init() {
  p_name         = "视频选项";
  p_show_str     = make_imgui_name(this, "不上传", "上传视频", "不上传源文件");
  p_image_create = new_object<image_sequence_async>();
}

bool comm_file_image_to_move::set_data(const entt::handle& in_data) {
  if (in_data.any_of<assets_path_vector>()) {
    p_root     = in_data;
    p_image    = p_image_create->set_path(p_root);
    p_out_file = p_image->get_out_path();
    p_text     = fmt::format("本地生成路径 {}", p_out_file);
  } else {
    p_root = entt::handle{};
  }
  return false;
}

bool comm_file_image_to_move::updata_file() {
  /// @todo 合成视频后需要上传视频
  if (*p_not_up_file)
    return true;

  if (*p_not_up_source_file)
    p_root.get<assets_path_vector>().get().clear();
  p_root.get<assets_path_vector>().add_file(p_out_file);
  p_root.get<assets_file>().up_version();
  p_root.patch<database_stauts>(database_set_stauts<need_save>{});
  auto k_up = doodle_lib::Get().get_rpc_file_system_client()->upload(p_root.get<assets_path_vector>().make_up_path());
  (*k_up)();

  return true;
}

bool comm_file_image_to_move::render() {
  if (p_root) {
    imgui::Checkbox(p_show_str["不上传"].c_str(), p_not_up_file.get());
    imgui::SameLine();
    imgui::Checkbox(p_show_str["不上传源文件"].c_str(), p_not_up_file.get());

    imgui::Text(p_text.c_str());

    if (imgui::Button(p_show_str["上传视频"].c_str())) {
      auto k_term = p_image_create->create_video();
      if (*p_not_up_file)
        return true;

      k_term->sig_finished.connect([k_not_up = *p_not_up_file, k_out_file = p_out_file, k_root = p_root]() {
        if (k_not_up)
          return;
        k_root.get<assets_path_vector>().add_file_raw(k_out_file);
        k_root.get<assets_file>().up_version();
        k_root.patch<database_stauts>(database_set_stauts<need_save>{});
        auto k_up = doodle_lib::Get().get_rpc_file_system_client()->upload(k_root.get<assets_path_vector>().make_up_path());
        (*k_up)();
      });
    }
  }
  return false;
}
}  // namespace doodle
