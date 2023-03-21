//
// Created by TD on 2021/11/22.
//

#pragma once

#include "doodle_lib/doodle_lib_fwd.h"

#include "main/maya_plug_fwd.h"

namespace doodle::maya_plug {
class create_hud_node {
 public:
  create_hud_node();
  [[nodiscard]] bool hide(bool hide) const;

  bool operator()() const;
};

}  // namespace doodle::maya_plug
