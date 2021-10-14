//
// Created by TD on 2021/10/14.
//

#pragma once

#include <doodle_lib/gui/action/command.h>

namespace doodle::maya_plug {

class reference_attr_setting : public command_base {
 public:
  reference_attr_setting();
  bool render() override;
};

}  // namespace doodle::maya_plug
