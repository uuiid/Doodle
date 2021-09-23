//
// Created by TD on 2021/9/23.
//

#include "attribute_factory_interface.h"

#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/libWarp/imgui_warp.h>

namespace doodle {
attr_project::attr_project()
    : p_prj() {
}

void attr_project::render() {
  

}

void attr_project::show_attribute(const ProjectPtr& in) {
  if (in != p_prj)
    p_prj = in;
}

}  // namespace doodle
