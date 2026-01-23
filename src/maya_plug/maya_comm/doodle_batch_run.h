#pragma once
#include <maya_plug/main/maya_plug_fwd.h>

#include "maya/MApiNamespace.h"
#include "maya/MDGModifier.h"
#include <maya/MSelectionList.h>

namespace doodle::maya_plug {
namespace doodle_batch_run_ns {
constexpr char doodle_batch_run[]{"doodle_batch_run"};
}
MSyntax doodle_batch_run_syntax();
class doodle_batch_run
    : public TemplateAction<doodle_batch_run, doodle_batch_run_ns::doodle_batch_run, doodle_batch_run_syntax> {
 public:
  doodle_batch_run();
  ~doodle_batch_run() override;
  [[maybe_unused]] MStatus doIt(const MArgList& in_list) override;

 private:
  class impl;
  std::unique_ptr<impl> ptr;
};
}  // namespace doodle::maya_plug