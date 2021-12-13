//
// Created by TD on 2021/12/13.
//

#include "cam_comm.h"

#include <maya/MArgParser.h>
#include <maya/MAnimControl.h>

#define doodle_startTime "-st"
#define doodle_endTime "-et"

#define doodle_startTime_long "-startTime"
#define doodle_endTime_long "-endTime"
namespace doodle::maya_plug {

MSyntax export_camera_syntax() {
  MSyntax syntax{};
  syntax.addFlag(doodle_startTime, doodle_startTime_long, MSyntax::kTime);
  syntax.addFlag(doodle_endTime, doodle_endTime_long, MSyntax::kTime);
  return syntax;
}
MStatus export_camera_command::doIt(const MArgList &in_arg) {
  MStatus k_s{};
  MArgParser k_prase{syntax(), in_arg, &k_s};
  MTime k_start{MAnimControl::minTime()};
  MTime k_end = MAnimControl::maxTime();
  if (k_prase.isFlagSet(doodle_startTime, &k_s)) {
    DOODLE_CHICK(k_s);
    k_s = k_prase.getFlagArgument(doodle_startTime, 0, k_start);
    DOODLE_CHICK(k_s);
  }
  if (k_prase.isFlagSet(doodle_endTime, &k_s)) {
    DOODLE_CHICK(k_s);
    k_s = k_prase.getFlagArgument(doodle_endTime, 0, k_end);
    DOODLE_CHICK(k_s);
  }





  return k_s;
}
}  // namespace doodle::maya_plug
