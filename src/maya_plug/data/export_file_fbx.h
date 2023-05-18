//
// Created by td_main on 2023/4/27.
//
#pragma once

#include <doodle_core/core/file_sys.h>

#include <maya_plug/data/sequence_to_blend_shape.h>
#include <maya_plug/maya_plug_fwd.h>

#include "entt/entity/fwd.hpp"
#include <maya/MApiNamespace.h>
#include <maya/MTime.h>
#include <optional>
#include <string>

namespace doodle::maya_plug {
class sequence_to_blend_shape;
class reference_file;

class export_file_fbx {
 private:
  void bake_anim(const MTime& in_start, const MTime& in_end, const MDagPath& in_path);
  void cloth_to_blendshape(
      const MTime& in_start, const MTime& in_end, const std::vector<MDagPath>& in_path,
      const std::optional<MDagPath>& in_parent_path = {}
  );

  std::string m_namespace_;
  std::vector<sequence_to_blend_shape> blend_list{};

 public:
  export_file_fbx() = default;

  void export_anim(const entt::handle_view<reference_file, generate_file_path_ptr>& in_handle_view);
  void export_sim(const entt::handle_view<reference_file, generate_file_path_ptr>& in_handle_view);
  void export_cam(const entt::handle_view<generate_file_path_ptr>& in_handle_view);
};

}  // namespace doodle::maya_plug
