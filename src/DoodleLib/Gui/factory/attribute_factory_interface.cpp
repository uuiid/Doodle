//
// Created by TD on 2021/9/23.
//

#include "attribute_factory_interface.h"

namespace doodle {
attr_project::attr_project()
    : p_prj() {
}

void attr_project::set_project(const ProjectPtr& in_prj) {
  p_prj = in_prj;
}
void attr_project::render() {
  
}
}  // namespace doodle
