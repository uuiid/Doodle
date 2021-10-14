#include <MotionMayaPlugInit.h>
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/lib_warp/std_warp.h>
#include <maya/MApiNamespace.h>
#include <maya/MObject.h>
namespace doodle::MayaPlug {
doodleCreate::doodle_data* doodleCreate::d_ptr_ = nullptr;

struct doodleCreate::doodle_data {
 public:
  static doodle_lib_ptr p_doodle_lib;
};
doodle_lib_ptr doodleCreate::doodle_data::p_doodle_lib = nullptr;

doodleCreate::doodleCreate()
    : MPxCommand() {
}

doodleCreate::~doodleCreate() = default;

void* doodleCreate::create() {
  return new doodleCreate();
}

MStatus doodleCreate::doIt(const MArgList& list) {
  using namespace doodle;
  if (!doodleCreate::d_ptr_->p_doodle_lib) {
    doodleCreate::d_ptr_->p_doodle_lib = make_doodle_lib();
    doodleCreate::d_ptr_->p_doodle_lib->init_gui();
  }
  doodle_app::make_this()->run();

  return MStatus::kFailure;
}

bool doodleCreate::isUndoable() const {
  return false;
}

void doodleCreate::clear_() {
  doodle::core_set::getSet().p_stop = true;
  doodleCreate::d_ptr_->p_doodle_lib.reset();
  delete d_ptr_;
}

}  // namespace doodle::MayaPlug

////////////////////////////////
