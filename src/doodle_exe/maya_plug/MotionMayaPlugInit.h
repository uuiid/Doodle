#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <maya/MApiNamespace.h>
#include <maya/MGlobal.h>
#include <maya/MPxCommand.h>

namespace doodle::MayaPlug {
class doodleCreate : public MPxCommand {
 public:
  doodleCreate();
  ~doodleCreate() override;

  static void* create();

  virtual MStatus doIt(const MArgList& list) override;
  virtual bool isUndoable() const override;

  static void clear_();

 private:
  struct doodle_data;
  static doodle_data* d_ptr_;
};
}  // namespace doodle::MayaPlug
