//
// Created by TD on 2022/1/20.
//

#pragma once
#include <doodle_lib/metadata/project.h>
#include <doodle_lib/gui/gui_ref/path.h>

namespace doodle{
namespace gui {
template <>
struct adl_render<project> : adl_render_data<project> {
  adl_render<FSys::path> p_path_render;
  bool render(const entt::handle& in_handle);
};
}  // namespace gui
}
