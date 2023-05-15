//
// Created by td_main on 2023/4/27.
//
#pragma once

#include <doodle_core/core/file_sys.h>

#include <maya_plug/maya_plug_fwd.h>

#include "entt/entity/fwd.hpp"
#include <maya/MTime.h>

namespace doodle::maya_plug {

class reference_file;
class export_file_fbx {
 public:
  export_file_fbx() = default;

  void export_anim(const entt::handle_view<reference_file, generate_file_path_ptr>& in_handle_view);
  void export_sim(const entt::handle_view<reference_file, generate_file_path_ptr>& in_handle_view);
  void export_cam(const entt::handle_view<generate_file_path_ptr>& in_handle_view);
};

}  // namespace doodle::maya_plug
