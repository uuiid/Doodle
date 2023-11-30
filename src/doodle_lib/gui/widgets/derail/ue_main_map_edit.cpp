//
// Created by TD on 2023/11/21.
//

#include "ue_main_map_edit.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/main_map.h>
#include <doodle_core/platform/win/windows_alias.h>

#include "imgui.h"
namespace doodle::gui::render {

void ue_main_map_edit::test_is_ue_dir(const FSys::path &in_path) {
  if (!FSys::exists(in_path)) return;
  if (!FSys::is_directory(in_path)) return;
  for (auto &&l_file : FSys::directory_iterator(in_path)) {
    if (l_file.path().extension() == ".uproject") {
      is_ue_dir_ = true;
      return;
    }
  }
  is_ue_dir_ = false;
}

void ue_main_map_edit::init(const entt::handle &in_handle) {
  if (render_id_ == in_handle) return;

  if (in_handle.all_of<assets_file>()) {
    test_is_ue_dir(in_handle.get<assets_file>().path_attr());
  }
  if (in_handle.all_of<ue_main_map>()) {
    map_path_ = in_handle.get<ue_main_map>().map_path_;
  } else
    map_path_ = "从ue中选择主要关卡, 并复制后点击按扭"s;

  render_id_ = in_handle;
}

bool ue_main_map_edit::render(const entt::handle &in_handle_view) {
  init(in_handle_view);
  bool on_change = false;
  if (!is_ue_dir_) return on_change;

  dear::Text(map_path_);
  ImGui::SameLine();
  if (ImGui::Button(*map_path_id)) {
    if (auto l_str = win::get_clipboard_data_str(); l_str.starts_with("/Script/Engine.World'/Game")) {
      map_path_ = l_str.substr(21, l_str.find('.', 26) - 21);
      in_handle_view.emplace_or_replace<ue_main_map>(map_path_);
      on_change = true;
    } else {
      map_path_ = fmt::format("{}", "剪切板中的格式, 不符合要求, 不是ue格式");
    }
  }
  if (auto l_tip = dear::ItemTooltip{}) {
    dear::Text("从ue中选择主要关卡, 并复制后点击按扭"s);
  }

  return on_change;
}

}  // namespace doodle::gui::render