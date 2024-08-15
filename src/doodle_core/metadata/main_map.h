//
// Created by TD on 2023/11/21.
//

#pragma once
#include <doodle_core/core/file_sys.h>

#include <entt/entt.hpp>
namespace doodle {
class assets_file;
class ue_main_map {
 public:
  static void find_ue_project_file(const entt::handle_view<assets_file, ue_main_map>& in_handle_view);
  static FSys::path find_ue_project_file(const FSys::path& in_path);
};
}  // namespace doodle
