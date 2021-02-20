#include <MayaPlug/MotionMayaPlugInit.h>

#include <Maya/MApiNamespace.h>
#include <Maya/MObject.h>

#include <lib/ui/MotionMainUI.h>

namespace doodle::MayaPlug {

doodle::motion::ui::MotionMainUI* doodleCreate::p_ui{nullptr};

doodleCreate::doodleCreate()
    : MPxCommand() {
}

doodleCreate::~doodleCreate() {
}

void* doodleCreate::create() {
  return new doodleCreate();
}

MStatus doodleCreate::doIt(const MArgList& list) {
  if (!p_ui) {
    p_ui = new doodle::motion::ui::MotionMainUI();
  }

  p_ui->showNormal();
  p_ui->raise();

  return MStatus::kFailure;
}

bool doodleCreate::isUndoable() const {
  return false;
}

void doodleCreate::clear_() {
  delete p_ui;
}

}  // namespace doodle::MayaPlug

////////////////////////////////
