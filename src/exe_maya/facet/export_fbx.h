//
// Created by td_main on 2023/4/25.
//
#pragma once
#include <doodle_core/core/app_base.h>
#include <doodle_core/metadata/image_size.h>

#include "entt/entity/fwd.hpp"
#include "exe_maya/core/maya_lib_guard.h"
#include "maya/MApiNamespace.h"
#include <cstdint>
#include <maya/MTime.h>
#include <memory>
#include <string>
#include <vector>
namespace doodle::maya_plug {
class export_fbx_facet final {
  static constexpr auto config{"export_fbx_config"};

  MTime anim_begin_time_{};
  std::double_t film_aperture_{};
  image_size size_{};

  void create_ref_file();
  void export_fbx();
  void rig_file_export();

  void play_blast();
  std::vector<entt::handle> ref_files_{};
  std::vector<entt::handle> cloth_lists_{};
  // 输出结果路径
  FSys::path out_path_file_{};

 public:
  export_fbx_facet()  = default;
  ~export_fbx_facet() = default;

  bool post(const nlohmann::json& in_argh);
};

}  // namespace doodle::maya_plug
