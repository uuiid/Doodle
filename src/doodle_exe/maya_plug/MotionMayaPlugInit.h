#pragma once

#include <maya/MGlobal.h>
#include <maya/MApiNamespace.h>

#include <maya/MPxCommand.h>

namespace doodle::motion::ui {
class MotionMainUI;
};

namespace doodle::MayaPlug {
class doodleCreate : public MPxCommand {
 public:
  doodleCreate();
  virtual ~doodleCreate();

  static void* create();

  virtual MStatus doIt(const MArgList& list) override;
  virtual bool isUndoable() const override;

  static void clear_();

 private:
  static doodle::motion::ui::MotionMainUI* p_ui;
};
}  // namespace doodle::MayaPlug