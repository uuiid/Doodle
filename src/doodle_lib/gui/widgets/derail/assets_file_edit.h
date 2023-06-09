//
// Created by td_main on 2023/6/9.
//

#pragma once

#include "doodle_core/metadata/assets_file.h"
#include <doodle_core/doodle_core_fwd.h>

#include "doodle_app/gui/base/ref_base.h"

#include "entt/entity/fwd.hpp"
#include <cstdint>
#include <string>
namespace doodle::gui::render {
class assets_file_edit_t {
 public:
  assets_file_edit_t() = default;
  explicit assets_file_edit_t(const entt::handle& in_handle_view) {
    if (in_handle_view.any_of<assets_file>()) {
      auto& l_assets_file = in_handle_view.get<assets_file>();
      path                = l_assets_file.path_attr().generic_string();
      name                = l_assets_file.name_attr();
      version             = l_assets_file.version_attr();
      user                = l_assets_file.user_attr();
    };
  };
  std::string path{};
  std::string name{};
  std::int32_t version{};
  entt::handle user{};

  gui_cache_name_id path_id{"路径"s};
  gui_cache_name_id name_id{"名称"s};
  gui_cache_name_id version_id{"版本"s};
  gui_cache_name_id user_id{"用户"s};
};
bool assets_file_edit(const entt::handle& in_handle_view);

}  // namespace doodle::gui::render
