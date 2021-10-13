#include <maya/MApiNamespace.h>
#include <maya/MObject.h>
#include <maya/MQtUtil.h>
#include <MotionMayaPlugInit.h>


namespace doodle::MayaPlug {

doodleCreate::doodleCreate()
    : MPxCommand() {
}

doodleCreate::~doodleCreate() = default;

void* doodleCreate::create() {
  return new doodleCreate();
}

MStatus doodleCreate::doIt(const MArgList& list) {

  return MStatus::kFailure;
}

bool doodleCreate::isUndoable() const {
  return false;
}

void doodleCreate::clear_() {

}

}  // namespace doodle::MayaPlug

////////////////////////////////
