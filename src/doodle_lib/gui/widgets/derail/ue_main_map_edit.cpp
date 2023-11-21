//
// Created by TD on 2023/11/21.
//

#include "ue_main_map_edit.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/main_map.h>

#include "imgui.h"
namespace doodle::gui::render {

void ue_main_map_edit::test_is_ue_dir(const FSys::path &in_path) {
  for (auto &&l_file : FSys::directory_iterator(in_path)) {
    if (l_file.path().extension() == ".uproject") {
      is_ue_dir_ = true;
    }
  }
  is_ue_dir_ = false;
}

void ue_main_map_edit::init(const entt::handle &in_handle) {
  if (render_id_ == in_handle) return;

  if (in_handle.all_of<assets_file, ue_main_map>()) {
    test_is_ue_dir(in_handle.get<assets_file>().path_attr());
  }

  render_id_ = in_handle;
}

bool ue_main_map_edit::render(const entt::handle &in_handle_view) {
  init(in_handle_view);
  bool on_change = false;
  if (!is_ue_dir_) return on_change;

  if (ImGui::InputText(*map_path_id, &map_path_)) {
    if (map_path_.starts_with("/Script/Engine.World'/Game")) {
      map_path_                                     = map_path_.substr(24, map_path_.size() - 24 - 1);
      in_handle_view.patch<ue_main_map>().map_path_ = map_path_;
      on_change                                     = true;
    }
  }
  if (auto l_tip = dear::ItemTooltip{}) {
    dear::Text("在ue中选择主要关卡, 并复制到输入框中"s);
  }

  return on_change;
}

}  // namespace doodle::gui::render