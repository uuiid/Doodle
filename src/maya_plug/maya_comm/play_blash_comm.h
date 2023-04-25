//
// Created by TD on 2021/12/13.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>

namespace doodle::maya_plug {
namespace {
constexpr char comm_play_blast_maya_name[] = "comm_play_blast_maya";
}
namespace details {
MSyntax comm_play_blast_maya_syntax();
}
class comm_play_blast_maya
    : public doodle::TemplateAction<
          comm_play_blast_maya, comm_play_blast_maya_name, details::comm_play_blast_maya_syntax> {
 public:
  MStatus doIt(const MArgList& in_arg) override;
};

constexpr char create_hud_node_maya_name[] = "create_hud_node_maya";
class create_hud_node_maya : public TemplateAction<create_hud_node_maya, create_hud_node_maya_name> {
 public:
  MStatus doIt(const MArgList& in_arg) override;
};

}  // namespace doodle::maya_plug
