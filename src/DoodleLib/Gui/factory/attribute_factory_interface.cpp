//
// Created by TD on 2021/9/23.
//

#include "attribute_factory_interface.h"

#include <DoodleLib/Gui/action/command_meta.h>
#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/libWarp/imgui_warp.h>
namespace doodle {
attr_project::attr_project()
    : p_prj(),
      p_comm(new_object<comm_project_add>()) {
}

void attr_project::render() {
  p_comm->run(nullptr, p_prj);
}

void attr_project::show_attribute(const ProjectPtr& in) {
  if (in != p_prj) {
    p_prj = in;
  }
}

}  // namespace doodle
