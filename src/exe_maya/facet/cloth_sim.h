//
// Created by td_main on 2023/4/25.
//
#pragma once

#include <cstdint>
#include <string>

namespace doodle::maya_plug {
class cloth_sim final {
  std::string files_attr{};
  bool is_init{};
  static constexpr auto config{"cloth_sim_config"};

  std::int32_t anim_begin_time_{};

  void create_ref_file();
  void replace_ref_file();
  void create_cloth();
  void sim();
  void export_fbx();
  void export_abc();
  void play_blast();

 public:
  cloth_sim() = default;
  ~cloth_sim();

  const std::string& name() const noexcept;
  bool post();
  void add_program_options();
};

}  // namespace doodle
