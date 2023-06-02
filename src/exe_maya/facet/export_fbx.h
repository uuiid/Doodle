//
// Created by td_main on 2023/4/25.
//
#pragma once

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

  void create_ref_file();
  void export_fbx();

  void play_blast();
  std::shared_ptr<maya_lib_guard> lib_guard_{};
  std::vector<entt::handle> ref_files_{};
  std::vector<entt::handle> cloth_lists_{};

 public:
  export_fbx_facet()  = default;
  ~export_fbx_facet() = default;

  [[nodiscard]] const std::string& name() const noexcept;
  bool post();
  void add_program_options();
};

}  // namespace doodle::maya_plug
