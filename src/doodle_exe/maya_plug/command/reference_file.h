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




  reference_file();

};

}  // namespace doodle::maya_plug
