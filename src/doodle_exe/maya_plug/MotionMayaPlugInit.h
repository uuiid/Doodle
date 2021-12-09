#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <maya/MGlobal.h>
#include <maya_plug/maya_plug_fwd.h>
namespace doodle::MayaPlug {
constexpr char doodleCreate_name[] = "doodleCreate";
class doodleCreate : public TemplateAction<
                         doodleCreate,
                         doodleCreate_name > {
 public:
  doodleCreate();
  ~doodleCreate() override;
  virtual MStatus doIt(const MArgList& list) override;
};
}  // namespace doodle::MayaPlug
