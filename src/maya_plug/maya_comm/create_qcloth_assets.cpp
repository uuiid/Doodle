//
// Created by TD on 2022/7/20.
//

#include "create_qcloth_assets.h"

namespace doodle::maya_plug {
MSyntax create_qcloth_assets_ns::syntax() {
  return MSyntax();
}
MStatus create_qcloth_assets::doIt(const MArgList& in_arg) {
  return redoIt();
}
MStatus create_qcloth_assets::undoIt() {
  return MStatus();
}
MStatus create_qcloth_assets::redoIt() {
  return MStatus();
}
bool create_qcloth_assets::isUndoable() const {
  return true;
}

}  // namespace doodle::maya_plug
