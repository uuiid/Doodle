//
// Created by TD on 2021/12/29.
//

#pragma once
#include <main/maya_plug_fwd.h>
namespace doodle::maya_plug {
namespace {
constexpr char afterimage_comm_name[] = "doodle_afterimage";
}
class afterimage_comm : public TemplateAction<afterimage_comm, afterimage_comm_name, null_syntax_t> {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  afterimage_comm();
  ~afterimage_comm() override;

  MStatus doIt(const MArgList&) override;
  [[maybe_unused]] MStatus undoIt() override;
  [[maybe_unused]] MStatus redoIt() override;
};

}  // namespace doodle::maya_plug
