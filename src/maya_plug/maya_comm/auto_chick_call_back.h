//
// Created by TD on 2022/4/26.
//

#pragma once
#include <maya_plug/main/maya_plug_fwd.h>

namespace doodle::maya_plug {

class auto_chick_call_back {
 public:
  static void add_call_back();
  static void delete_call_back();
  static void show_windows();
};

}  // namespace doodle::maya_plug
