//
// Created by TD on 2021/11/22.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <maya/MPxCommand.h>

namespace doodle::maya_plug {
class create_hud_node {
 public:
  create_hud_node();
  bool hide(bool hide) const;
  
  bool operator()() const;

};


class create_hud_node_maya : public MPxCommand{
 public:
  static MString comm_name;
  MStatus doIt(const MArgList& in_arg) override;
  static void* creator();
};
}  // namespace doodle::maya_plug