//
// Created by td_main on 2023/4/25.
//
#pragma once

#include "doodle_core/metadata/image_size.h"
#include <doodle_core/core/app_base.h>

#include <doodle_lib/exe_warp/maya_exe.h>

#include <maya_plug/data/cloth_interface.h>
#include <maya_plug/data/reference_file.h>

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
  std::double_t film_aperture_{};
  image_size size_{};
  void create_ref_file();
  void replace_ref_file();
  void create_cloth();
  void set_cloth_attr();

  /// 删除缓存后, 重新生成缓存, 并且保存文件
  void sim();
  /// 有解算就使用, 没有解算就生成, 并且不保存文件
  void touch_sim();
  void export_abc();
  void play_blast();
  void export_anim_file();
  void write_config();
  std::vector<reference_file> ref_files_{};
  std::vector<reference_file> all_ref_files_{};
  std::vector<cloth_interface> cloth_lists_{};
  std::map<std::string, FSys::path> sim_file_map_{};
  std::map<reference_file, std::vector<FSys::path>> out_and_ref_file_map_{};
  FSys::path camera_path_{};
  FSys::path out_path_file_;
  maya_exe_ns::maya_out_arg out_arg_{};

 public:
  cloth_sim()  = default;
  ~cloth_sim() = default;

  bool post(const nlohmann::json& in_argh);
};

}  // namespace doodle::maya_plug
