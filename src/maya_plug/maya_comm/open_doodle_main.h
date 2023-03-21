//
// Created by TD on 2021/12/13.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>
namespace doodle::maya_plug {
namespace {
constexpr char doodleCreate_name[] = "doodleCreate";
}
class open_doodle_main : public TemplateAction<open_doodle_main, doodleCreate_name> {
 public:
  open_doodle_main();
  ~open_doodle_main() override;
  virtual MStatus doIt(const MArgList& list) override;
};
}  // namespace doodle::maya_plug
