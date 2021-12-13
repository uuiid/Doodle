//
// Created by TD on 2021/12/13.
//

#pragma once

#include <maya_plug/maya_plug_fwd.h>

namespace doodle::maya_plug {

constexpr char comm_play_blast_maya_name[] = "comm_play_blast_maya";
MSyntax comm_play_blast_maya_syntax();
class comm_play_blast_maya : public doodle::TemplateAction<
                                 comm_play_blast_maya,
                                 comm_play_blast_maya_name,
                                 comm_play_blast_maya_syntax> {
 public:
  MStatus doIt(const MArgList& in_arg) override;
};
}
