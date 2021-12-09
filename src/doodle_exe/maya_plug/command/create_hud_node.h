//
// Created by TD on 2021/11/22.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <maya/MTemplateCommand.h>

namespace doodle::maya_plug {
class create_hud_node {
 public:
  create_hud_node();
  bool hide(bool hide) const;

  bool operator()() const;
};
constexpr char create_hud_node_maya_name[] = "create_hud_node_maya";
class create_hud_node_maya : public MTemplateAction<
                                 create_hud_node_maya,
                                 create_hud_node_maya_name,
                                 MTemplateCommand_nullSyntax> {
 public:
  MStatus doIt(const MArgList& in_arg) override;
};
}  // namespace doodle::maya_plug
