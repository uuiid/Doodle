//
// Created by TD on 2023/11/21.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include "doodle_app/gui/base/ref_base.h"
#include <doodle_app/lib_warp/imgui_warp.h>

namespace doodle::gui::render {

class ue_main_map_edit {
  gui_cache_name_id map_path_id{"主要项目路径"s};
  std::string u_project_str_{};
  FSys::path u_project_path_{};

  bool is_ue_dir_{false};

  entt::handle render_id_{};
  void init(const entt::handle& in_handle);
  void test_is_ue_dir(const FSys::path& in_path);

 public:
  ue_main_map_edit() = default;

  bool render(const entt::handle& in_handle_view);
};

}  // namespace doodle::gui::render
