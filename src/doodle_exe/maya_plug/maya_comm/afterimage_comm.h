//
// Created by TD on 2021/12/29.
//

#pragma once
#include <maya_plug/maya_plug_fwd.h>
namespace doodle::maya_plug {
namespace {
constexpr char afterimage_comm_name[] = "doodle_afterimage";
}
class afterimage_comm : public TemplateAction<
                            afterimage_comm,
                            afterimage_comm_name,
                            null_syntax_t> {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  afterimage_comm();
  ~afterimage_comm();

  MStatus doIt(const MArgList&) override;
  MStatus undoIt() override;
  MStatus redoIt() override;
  bool isUndoable() const override;

};

}  // namespace doodle::maya_plug
