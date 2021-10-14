#include <MotionMayaPlugInit.h>
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/lib_warp/std_warp.h>
#include <maya/MApiNamespace.h>
#include <maya/MObject.h>
namespace doodle::MayaPlug {
doodle_lib_ptr doodleCreate::p_doodle_lib                     = nullptr;
::std::unique_ptr<::doodle::doodle_app> doodleCreate::app_ptr = nullptr;

doodleCreate::doodleCreate()
    : MPxCommand() {
}

doodleCreate::~doodleCreate() = default;

void* doodleCreate::create() {
  return new doodleCreate();
}

MStatus doodleCreate::doIt(const MArgList& list) {
  using namespace doodle;
  if (!doodleCreate::p_doodle_lib) {
    doodleCreate::p_doodle_lib = make_doodle_lib();
    doodleCreate::p_doodle_lib->init_gui();
    doodleCreate::app_ptr = doodle_app::make_this();
    std::thread([]() {
      return doodleCreate::app_ptr->run();
    }).detach();
  }
  if (core_set::getSet().p_stop) {
    core_set::getSet().p_stop = false;
    std::thread([]() {
      return doodleCreate::app_ptr->run();
    }).detach();
  }

  return MStatus::kFailure;
}

bool doodleCreate::isUndoable() const {
  return false;
}

void doodleCreate::clear_() {
  doodle::core_set::getSet().p_stop = true;
  doodleCreate::app_ptr.reset();
  doodleCreate::p_doodle_lib.reset();
}

}  // namespace doodle::MayaPlug

////////////////////////////////
