#include "MotionMayaPlugInit.h"

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/lib_warp/std_warp.h>
#include <maya/MApiNamespace.h>
#include <maya/MObject.h>
#include <maya_plug/gui/maya_plug_app.h>
namespace doodle::MayaPlug {
doodleCreate::doodle_data* doodleCreate::d_ptr_ = nullptr;

struct doodleCreate::doodle_data {
 public:
  static doodle_lib_ptr p_doodle_lib;
  static std::shared_ptr<doodle_app> p_doodle_app;
};
doodle_lib_ptr doodleCreate::doodle_data::p_doodle_lib              = nullptr;
std::shared_ptr<doodle_app> doodleCreate::doodle_data::p_doodle_app = nullptr;

doodleCreate::doodleCreate()
    : MPxCommand() {
}

doodleCreate::~doodleCreate() = default;

void* doodleCreate::create() {
  return new doodleCreate();
}

MStatus doodleCreate::doIt(const MArgList& list) {
  using namespace doodle;
  if (!doodleCreate::d_ptr_) {
    doodleCreate::d_ptr_ = new doodle_data;
  }
  if (doodleCreate::d_ptr_ && !doodleCreate::d_ptr_->p_doodle_lib) {
    doodleCreate::d_ptr_->p_doodle_lib = make_doodle_lib();
    doodleCreate::d_ptr_->p_doodle_lib->init_gui();
  }
  if (!(d_ptr_->p_doodle_app)) {
    d_ptr_->p_doodle_app = new_object<::doodle::maya_plug::maya_plug_app>();
  }
  if (!d_ptr_->p_doodle_app->valid()) {
    d_ptr_->p_doodle_app = new_object<::doodle::maya_plug::maya_plug_app>();
  }
  if (d_ptr_->p_doodle_app->p_done) {
    d_ptr_->p_doodle_app->p_done = false;
  }
  d_ptr_->p_doodle_app->run();

  return MStatus::kFailure;
}

bool doodleCreate::isUndoable() const {
  return false;
}

void doodleCreate::clear_() {
  if (doodleCreate::d_ptr_) {
    if (d_ptr_->p_doodle_app)
      d_ptr_->p_doodle_app->p_done = true;
    doodle::core_set::getSet().p_stop = true;
    d_ptr_->p_doodle_app.reset();
    doodleCreate::d_ptr_->p_doodle_lib.reset();
    delete d_ptr_;
  }
}

}  // namespace doodle::MayaPlug

////////////////////////////////
