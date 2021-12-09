#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <maya/MGlobal.h>
#include <maya/MTemplateCommand.h>

namespace doodle::MayaPlug {
constexpr char doodleCreate_name[] = "doodleCreate";
class doodleCreate : public MTemplateAction<
                         doodleCreate,
                         doodleCreate_name,
                         MTemplateCommand_nullSyntax> {
 public:
  doodleCreate();
  ~doodleCreate() override;
  virtual MStatus doIt(const MArgList& list) override;
};
}  // namespace doodle::MayaPlug
