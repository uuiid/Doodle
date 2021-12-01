//
// Created by TD on 2021/11/30.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle::maya_plug {

class reference_file {
 public:
  string path;
  bool use_sim;
  bool high_speed_sim;
  std::vector<string> k_collision_ref_file;
  std::vector<string> k_collision_model;

  uuid p_prj_ref;

  reference_file();
};

}  // namespace doodle::maya_plug
