//
// Created by TD on 2021/9/23.
//

#include "attribute_factory_interface.h"

#include <doodle_lib/gui/action/command_meta.h>
#include <doodle_lib/gui/action/command_video.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata_cpp.h>
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
    p_comm->set_data(p_prj);
  }
}

attr_assets::attr_assets()
    : p_data(),
      p_comm() {
  auto k_l = new_object<command_base_list>();
  k_l->p_list.push_back(new_object<comm_ass_season>());
  k_l->p_list.push_back(new_object<comm_ass_eps>());
  k_l->p_list.push_back(new_object<comm_ass_shot>());
  k_l->p_list.push_back(new_object<comm_assets>());
  k_l->p_list.push_back(new_object<comm_ass_ue4_create_shot>());
  k_l->p_list.push_back(new_object<comm_video>());
  p_comm = k_l;
}

void attr_assets::render() {
  p_comm->render();
}

void attr_assets::show_attribute(const episodes_ptr& in) {
  //  p_comm = new_object<comm_ass_eps>();
  p_comm->set_parent(in);
  p_comm->set_data(in);
}

void attr_assets::show_attribute(const project_ptr& in) {
  p_comm->set_parent(in);
  p_comm->set_data(nullptr);
}

void attr_assets::show_attribute(const shot_ptr& in) {
  //  p_comm = new_object<comm_ass_shot>();
  p_comm->set_parent(in);
  p_comm->set_data(in);
}

void attr_assets::show_attribute(const assets_ptr& in) {
  //  p_comm = new_object<comm_assets>();
  p_comm->set_parent(in);
  p_comm->set_data(in);
}

void attr_assets::show_attribute(const season_ptr& in) {
  p_comm->set_parent(in);
  p_comm->set_data(in);
}

attr_assets_file::attr_assets_file()
    : p_data(),
      p_comm() {
  auto k_l = new_object<command_base_list>();
  k_l->p_list.push_back(new_object<comm_ass_file_attr>());
  k_l->p_list.push_back(new_object<comm_files_select>());
  p_comm = k_l;
}

void attr_assets_file::render() {
  p_comm->render();
}

void attr_assets_file::show_attribute(const assets_file_ptr& in) {
  if (in->has_parent()) {
    p_comm->set_parent(in->get_parent());
    p_comm->set_data(in);
  }
}

}  // namespace doodle
