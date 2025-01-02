//
// Created by TD on 25-1-2.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle::maya_plug {

class inspect_file {
 public:
  bool post(const nlohmann::json& in_argh);
};

}  // namespace doodle::maya_plug