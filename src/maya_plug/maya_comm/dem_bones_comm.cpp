//
// Created by TD on 2022/6/30.
//

#include "dem_bones_comm.h"
namespace doodle::maya_plug {
namespace dem_bones_comm_ns {

MSyntax syntax() {
  MSyntax syntax{};
  syntax.addFlag("", "", MSyntax::kString);
  return syntax;
}
}  // namespace dem_bones_comm_ns
class dem_bones_comm::impl {
};
dem_bones_comm::dem_bones_comm()
    : p_i(std::make_unique<impl>()) {
}
void dem_bones_comm::get_arg(const MArgList& in_arg) {
}
MStatus dem_bones_comm::doIt(const MArgList& in_arg) {
  get_arg(in_arg);
  return TemplateAction::doIt(in_arg);
}

dem_bones_comm::~dem_bones_comm() = default;

}  // namespace doodle::maya_plug
