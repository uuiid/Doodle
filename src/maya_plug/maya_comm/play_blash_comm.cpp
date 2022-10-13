//
// Created by TD on 2021/12/13.
//

#include "play_blash_comm.h"

#include <maya_plug/data/create_hud_node.h>
#include <maya_plug/data/play_blast.h>

#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>

namespace doodle::maya_plug {
#define doodle_filepath "-fp"
#define doodle_startTime "-st"
#define doodle_endTime "-et"
#define doodle_filepath_long "-filepath"
#define doodle_startTime_long "-startTime"
#define doodle_endTime_long "-endTime"
#define doodle_width "-width"
#define doodle_height "-height"

MSyntax details::comm_play_blast_maya_syntax() {
  MSyntax syntax{};
  syntax.addFlag(doodle_filepath, doodle_filepath_long, MSyntax::kString);
  syntax.addFlag(doodle_startTime, doodle_startTime_long, MSyntax::kTime);
  syntax.addFlag(doodle_endTime, doodle_endTime_long, MSyntax::kTime);
  // syntax.addFlag("-w", doodle_width, MSyntax::kUnsigned);
  // syntax.addFlag("-h", doodle_height, MSyntax::kUnsigned);
  return syntax;
}

MStatus comm_play_blast_maya::doIt(const MArgList& in_arg) {
  MStatus k_s;
  play_blast k_p{};
  MArgDatabase l_database{syntax(), in_arg};

  DOODLE_LOG_INFO("开始从推测相机");
  k_p.conjecture_camera();

  DOODLE_LOG_INFO("开始推测集数和镜头");
  k_p.conjecture_ep_sc();

  if (l_database.isFlagSet(doodle_filepath, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    auto k_path = l_database.flagArgumentString(doodle_filepath, 0, &k_s);
    DOODLE_MAYA_CHICK(k_s);
    DOODLE_LOG_INFO("获得传入路径: {}", k_path);
    k_p.set_save_path(k_path.asUTF8());
  }

  DOODLE_LOG_INFO("开始拍屏");

  MTime k_start_time = MAnimControl::minTime();
  if (l_database.isFlagSet(doodle_startTime, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    k_start_time = l_database.flagArgumentMTime(doodle_startTime, 0, &k_s);
    DOODLE_MAYA_CHICK(k_s);
  } else {
    auto l_time  = g_reg()->ctx().at<project_config::base_config>().export_anim_time;
    k_start_time = MTime{boost::numeric_cast<std::double_t>(l_time), MTime::uiUnit()};
  }

  MTime k_end_time = MAnimControl::maxTime();
  if (l_database.isFlagSet(doodle_endTime, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    k_end_time = l_database.flagArgumentMTime(doodle_endTime, 0, &k_s);
    DOODLE_MAYA_CHICK(k_s);
  }

  k_s = k_p.play_blast_(k_start_time, k_end_time);
  if (MGlobal::mayaState(&k_s) == MGlobal::kInteractive) {
    DOODLE_MAYA_CHICK(k_s);
    FSys::open_explorer(k_p.get_out_path());
  }

  return k_s;
}

#undef doodle_filepath
#undef doodle_startTime
#undef doodle_endTime
#undef doodle_filepath_long
#undef doodle_startTime_long
#undef doodle_endTime_long
#undef doodle_width
#undef doodle_height

MStatus create_hud_node_maya::doIt(const MArgList& in_arg) {
  create_hud_node k_c{};
  return k_c() ? MStatus::kSuccess : MStatus::kFailure;
}
}  // namespace doodle::maya_plug
