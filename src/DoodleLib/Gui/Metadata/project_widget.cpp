//
// Created by TD on 2021/6/28.
//

#include "project_widget.h"

#include <Metadata/Project.h>
#include <core/MetadataSet.h>
namespace doodle {

project_widget::project_widget(nana::window in_window)
    : p_list_box(in_window) {
  p_list_box.append_header("名称");
  p_list_box.append_header("根目录");
  p_list_box.append_header("英文名称");

  auto& set = MetadataSet::Get();
  p_list_box.clear();
  for (auto& k_i : set.getAllProjects()) {
    p_list_box.at(0).append(k_i, true);
  }
  p_list_box.events().selected([](const nana::arg_listbox& in_) {
    in_.item.value<ProjectPtr>();
  });
}
nana::listbox& project_widget::get_listbox() {
  return p_list_box;
}

nana::listbox::oresolver& operator<<(nana::listbox::oresolver& oor, const ProjectPtr& prj) {
  oor << prj->showStr();
  oor << prj->getPath().generic_string();
  oor << prj->str();
  return oor;
}
nana::listbox::iresolver& operator>>(nana::listbox::iresolver& oor, ProjectPtr& prj) {
  return oor;
}
}  // namespace doodle
