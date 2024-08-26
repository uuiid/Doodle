//
// Created by td_main on 2023/4/27.
//
#pragma once

#include <doodle_core/core/file_sys.h>

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

 public:
  export_file_fbx() = default;

  FSys::path export_anim(
      const reference_file& in_ref, const generate_file_path_ptr in_gen_file, const MSelectionList& in_exclude = {}
  );

  FSys::path export_sim(const reference_file& in_ref, const generate_file_path_ptr in_gen_file);

  FSys::path export_cam(const generate_file_path_ptr& in_gen);

  FSys::path export_rig();
};

}  // namespace doodle::maya_plug
