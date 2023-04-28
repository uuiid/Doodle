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
class cloth_sim final {
  static constexpr auto config{"cloth_sim_config"};

  MTime anim_begin_time_{};
  MTime t_post_time_{};

  void create_ref_file();
  void replace_ref_file();
  void create_cloth();
  void sim();
  void export_fbx();
  void export_abc();
  void play_blast();
  std::shared_ptr<maya_lib_guard> lib_guard_{};
  std::vector<entt::handle> ref_files_{};
  std::vector<entt::handle> cloth_lists_{};

 public:
  cloth_sim()  = default;
  ~cloth_sim() = default;

  const std::string& name() const noexcept;
  bool post();
  void add_program_options();
};

}  // namespace doodle::maya_plug
