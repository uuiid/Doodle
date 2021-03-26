#include <MayaPlug/MotionMayaPlugInit.h>

#include <Maya/MApiNamespace.h>
#include <Maya/MObject.h>
#include <Maya/MQtUtil.h>

#include <lib/ui/MotionMainUI.h>
#include <QtWidgets/QMessageBox>

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
    try {
      p_ui = new doodle::motion::ui::MotionMainUI(mayaMainUI);
    } catch (const FSys::filesystem_error& err) {
      QMessageBox::warning(mayaMainUI, "错误:", QString{"无法找到目录: \n"}.append(QString::fromStdString(err.what())));
      MGlobal::displayError(err.what());
      return MStatus::kFailure;
    }
  }

  p_ui->showNormal();
  p_ui->raise();

  return MStatus::kFailure;
}

bool doodleCreate::isUndoable() const {
  return false;
}

void doodleCreate::clear_() {
  if (p_ui) {
    p_ui->deleteLater();
    delete p_ui;
  }
}

}  // namespace doodle::MayaPlug

////////////////////////////////
