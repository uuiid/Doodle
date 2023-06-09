//
// Created by td_main on 2023/6/9.
//

#pragma once

#include "doodle_core/metadata/assets_file.h"
#include <doodle_core/doodle_core_fwd.h>

#include "doodle_app/gui/base/ref_base.h"

#include "entt/entity/fwd.hpp"
#include <cstdint>
#include <memory>
#include <string>
namespace doodle::gui::render {
class select_all_user_t;
class assets_file_edit_t {
  entt::handle render_id{};

  std::string path{};
  std::string name{};
  std::int32_t version{};

  gui_cache_name_id path_id{"路径"s};
  gui_cache_name_id name_id{"名称"s};
  gui_cache_name_id version_id{"版本"s};
  gui_cache_name_id user_id{"用户"s};
  std::shared_ptr<select_all_user_t> user_edit{};

  void init(const entt::handle& in_handle);

 public:
  assets_file_edit_t() = default;

  bool render(const entt::handle& in_handle_view);
};

}  // namespace doodle::gui::render
