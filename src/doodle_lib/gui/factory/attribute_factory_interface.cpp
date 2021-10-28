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

void attr_project::show_attribute(const project* in) {

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

void attr_assets::show_attribute(const episodes* in) {

}

void attr_assets::show_attribute(const project* in) {

}

void attr_assets::show_attribute(const shot* in) {

}

void attr_assets::show_attribute(const assets* in) {

}

void attr_assets::show_attribute(const season* in) {

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

void attr_assets_file::show_attribute(const assets_file* in) {
}

}  // namespace doodle
