//
// Created by TD on 2021/9/18.
//

#include "command_files.h"

#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/core/open_file_dialog.h>
#include <doodle_lib/file_warp/image_sequence.h>
#include <doodle_lib/metadata/metadata_cpp.h>

namespace doodle {
comm_files_select::comm_files_select()
    : p_root(),
      p_use_relative(new_object<bool>(false)),
      p_file() {
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
    //    static bool use_folder{false};
    //    imgui::Checkbox("选择目录", &use_folder);
    //    imgui::SameLine();
    if (imgui::Button(p_show_str["添加文件"].c_str())) {
      g_main_loop().attach<file_dialog>(
          [this](const FSys::path& in_p) {
            p_file       = in_p;
            auto& k_path = p_root.get<assets_path_vector>();
            if (*p_use_relative)
              k_path.make_path(p_root, p_file);
            else
              k_path.make_path(p_root);
            //            p_comm_sub = k_path.add_file(p_file);
            if (p_comm_sub) {
              p_comm_sub->set_data(p_root);
            }
          },
          "获得目录");
    }
    imgui::SameLine();
    if (imgui::Checkbox(p_show_str["相对路径"].c_str(), p_use_relative.get())) {
      auto& k_path = p_root.get<assets_path_vector>();
      if (*p_use_relative)
        k_path.make_path(p_root, p_file);
      else
        k_path.make_path(p_root);
      //      p_comm_sub = k_path.add_file(p_file);
      if (p_comm_sub) {
        p_comm_sub->set_data(p_root);
      }
    }

    dear::ListBox{
        p_show_str["路径列表"].c_str(),
        ImVec2{-FLT_MIN, 5 * imgui::GetTextLineHeightWithSpacing()}} &&
        [this]() {
          for (auto& i : p_root.get_or_emplace<assets_path_vector>().list()) {
            auto str = fmt::format("{}", i);
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
  p_list_paths.get<time_point_wrap>().set_time(chrono::system_clock::now());


  return false;
}

bool comm_files_up::render() {
  if (p_list_paths) {
    if (!p_list_paths.get<assets_path_vector>().get_server_path().empty()) {
      if (imgui::Button(p_show_str["添加"].c_str())) {
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
  p_name     = "视频选项";
  p_show_str = make_imgui_name(this, "不上传", "上传视频", "不上传源文件");
}

bool comm_file_image_to_move::set_data(const entt::handle& in_data) {
  if (in_data.any_of<assets_path_vector>()) {
    p_root = in_data;
    p_text = fmt::format("本地生成路径 {}", p_out_file);
  } else {
    p_root = entt::handle{};
  }
  return false;
}

bool comm_file_image_to_move::updata_file() {
  /// @todo 合成视频后需要上传视频
  if (*p_not_up_file)
    return true;

  /// @todo 不上传源文件动作
  // if (*p_not_up_source_file)
  //   p_root.get<assets_path_vector>().clear();
  //  p_root.get<assets_path_vector>().add_file(p_out_file);
  p_root.get<assets_file>().up_version();
  p_root.get<time_point_wrap>().set_time(chrono::system_clock::now());
  p_root.patch<database_stauts>(database_set_stauts<need_save>{});


  return true;
}

bool comm_file_image_to_move::render() {
  if (p_root) {
    imgui::Checkbox(p_show_str["不上传"].c_str(), p_not_up_file.get());
    imgui::SameLine();
    imgui::Checkbox(p_show_str["不上传源文件"].c_str(), p_not_up_file.get());

    dear::Text(p_text);

    if (imgui::Button(p_show_str["上传视频"].c_str())) {
      if (*p_not_up_file)
        return true;
    }
  }
  return false;
}
comm_file_image_to_move::comm_file_image_to_move()
    : p_out_file(),
      p_not_up_file(new_object<bool>(false)),
      p_not_up_source_file(new_object<bool>(false)),
      p_root() {
  init();
}
}  // namespace doodle
