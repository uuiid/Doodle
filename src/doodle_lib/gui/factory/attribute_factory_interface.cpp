//
// Created by TD on 2021/9/23.
//

#include "attribute_factory_interface.h"

#include <doodle_lib/Gui/action/command_meta.h>
#include <doodle_lib/Metadata/metadata_cpp.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
namespace doodle {
attr_project::attr_project()
    : p_prj(),
      p_comm(new_object<comm_project_add>()) {
}

void attr_project::render() {
  p_comm->render();
}

void attr_project::show_attribute(const project_ptr& in) {
  if (in != p_prj) {
    p_prj = in;
    p_comm->add_data(nullptr, p_prj);
  }
}

attr_assets::attr_assets()
    : p_data(),
      p_comm(new_object<comm_ass>()) {
}

void attr_assets::render() {
  p_comm->render();
}

void attr_assets::show_attribute(const episodes_ptr& in) {
  //  p_comm = new_object<comm_ass_eps>();
  p_comm->add_data(in, in);
}

void attr_assets::show_attribute(const project_ptr& in) {
  p_comm->add_data(in, nullptr);
}

void attr_assets::show_attribute(const shot_ptr& in) {
  //  p_comm = new_object<comm_ass_shot>();
  p_comm->add_data(in, in);
}

void attr_assets::show_attribute(const assets_ptr& in) {
  //  p_comm = new_object<comm_assets>();
  p_comm->add_data(in, in);
}

void attr_assets::show_attribute(const season_ptr& in) {
  p_comm->add_data(in, in);
}

attr_assets_file::attr_assets_file()
    : p_data(),
      p_comm(new_object<comm_assets_file>()) {
}

void attr_assets_file::render() {
  p_comm->render();
}

// void attr_assets_file::show_attribute(const episodes_ptr& in) {
//   p_comm->add_data(in, nullptr);
// }

// void attr_assets_file::show_attribute(const shot_ptr& in) {
//   p_comm->add_data(in, nullptr);
// }

// void attr_assets_file::show_attribute(const assets_ptr& in) {
//   p_comm->add_data(in, nullptr);
// }

// void attr_assets_file::show_attribute(const season_ptr& in) {
//   p_comm->add_data(in, nullptr);
// }

void attr_assets_file::show_attribute(const assets_file_ptr& in) {
  if (in->has_parent())
    p_comm->add_data(in->get_parent(), in);
}

}  // namespace doodle
