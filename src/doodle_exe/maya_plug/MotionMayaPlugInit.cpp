#include "MotionMayaPlugInit.h"

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/lib_warp/std_warp.h>
#include <maya/MApiNamespace.h>
#include <maya/MObject.h>
#include <maya_plug/gui/maya_plug_app.h>
namespace doodle::MayaPlug {

doodleCreate::doodleCreate()  = default;

doodleCreate::~doodleCreate() = default;



MStatus doodleCreate::doIt(const MArgList& list) {
  using namespace doodle;
  // bool k_d = doodle::doodle_app::Get()->p_done;
  if (doodle::doodle_app::Get()->p_done) {
    doodle::doodle_app::Get()->loop_begin();
    doodle::doodle_app::Get()->p_done = false;
  }

  return MStatus::kSuccess;
}

}  // namespace doodle::MayaPlug

////////////////////////////////
