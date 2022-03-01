//
// Created by TD on 2022/2/28.
//

#include "clear_scene_comm.h"
namespace doodle::maya_plug {
namespace {
constexpr char syntax_select_long[] = "-select";
constexpr char syntax_select[]      = "-sl";


}  // namespace
MSyntax clear_scene_comm_syntax() {
  MSyntax syntax{};
  return MSyntax();
}

MStatus clear_scene_comm::doIt(const MArgList &) {
  MStatus k_s{};
  return k_s;
}
}  // namespace doodle::maya_plug
