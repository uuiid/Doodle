#include <MayaPlug/MotionMayaPlugInit.h>

#include <Maya/MApiNamespace.h>
#include <Maya/MObject.h>
#include <Maya/MQtUtil.h>

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
    auto mayaMainUI = MQtUtil::mainWindow();
    p_ui            = new doodle::motion::ui::MotionMainUI(mayaMainUI);
  }

  p_ui->showNormal();
  p_ui->raise();

  return MStatus::kFailure;
}

bool doodleCreate::isUndoable() const {
  return false;
}

void doodleCreate::clear_() {
  p_ui->deleteLater();
  delete p_ui;
}

}  // namespace doodle::MayaPlug

////////////////////////////////
