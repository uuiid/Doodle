//
// Created by TD on 2021/9/23.
//

#include "attribute_factory_interface.h"

#include <DoodleLib/Gui/action/command_meta.h>
#include <DoodleLib/Metadata/Metadata_cpp.h>
#include <DoodleLib/libWarp/imgui_warp.h>
namespace doodle {
attr_project::attr_project()
    : p_prj(),
      p_comm(new_object<comm_project_add>()) {
}

void attr_project::render() {
  p_comm->render();
}

void attr_project::show_attribute(const ProjectPtr& in) {
  if (in != p_prj) {
    p_prj = in;
    p_comm->add_data(nullptr, p_prj);
  }
}

attr_assets::attr_assets()
    : p_data(),
      p_comm() {
}

void attr_assets::render() {
  p_comm->render();
}

void attr_assets::show_attribute(const EpisodesPtr& in) {
  p_comm = new_object<comm_ass_eps>();
  p_comm->add_data(in, in);
}

void attr_assets::show_attribute(const ProjectPtr& in) {
  p_comm->add_data(in, nullptr);
}

void attr_assets::show_attribute(const ShotPtr& in) {
  p_comm = new_object<comm_ass_shot>();
  p_comm->add_data(in, in);
}

void attr_assets::show_attribute(const AssetsPtr& in) {
  p_comm = new_object<comm_assets>();
  p_comm->add_data(in, in);
}

void attr_assets::show_attribute(const season_ptr& in) {
  p_comm = new_object<comm_ass_season>();
  p_comm->add_data(in, in);
}

}  // namespace doodle
