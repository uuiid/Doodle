//
// Created by TD on 2022/1/20.
//

#include "project.h"
namespace doodle {
namespace gui {
bool adl_render<project>::render(const entt::handle& in_handle) {
  if (dear::InputText("名称: ", &data.p_name))
    in_handle.patch<project>([&](project& in) {
      in.p_name = data.p_name;
    });
  if (p_path_render.render(data.p_path))
    in_handle.patch<project>([&](project& in) {
      in.p_path = data.p_path;
    });
  return true;
}
}  // namespace gui
}  // namespace doodle
