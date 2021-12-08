//
// Created by TD on 2021/11/16.
//

#include "comm_play_blast.h"

#include <doodle_exe/maya_plug/command/play_blast.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/exception/exception.h>
#include <doodle_lib/file_warp/image_sequence.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <maya/M3dView.h>
#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFileIO.h>
#include <maya/MFnCamera.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MSyntax.h>
#include <maya_plug/command/create_hud_node.h>

namespace doodle::maya_plug {

comm_play_blast::comm_play_blast()
    : command_base(),
      use_conjecture_cam(true),
      p_save_path(core_set::getSet().get_cache_root("maya_play_blast").generic_string()),
      p_play_balst(new_object<play_blast>()) {
  p_show_str = make_imgui_name(this,
                               "保存路径",
                               "拍摄",
                               "拍屏",
                               "hud",
                               "选择相机",
                               "推测相机",
                               "打开文件夹",
                               "打开上一次拍屏");
}

bool comm_play_blast::render() {
  if (imgui::Button(p_show_str["拍屏"].c_str())) {
    if (use_conjecture_cam)
      p_play_balst->conjecture_camera();
    else
      p_play_balst->set_camera(p_camera_path);

    if (p_play_balst->conjecture_ep_sc()) {
      p_play_balst->set_save_dir(p_save_path);
      p_play_balst->play_blast_(MAnimControl::minTime(), MAnimControl::maxTime());
    } else {
      MString k_s{};
      k_s.setUTF8("无法分析路径得到镜头号和集数， 请重新设置文件路径");
      MGlobal::displayError(k_s);
    }
  }
  if (imgui::Button(p_show_str["hud"].c_str())) {
    create_hud_node k_c{};
    k_c();
  }

  imgui::Checkbox(p_show_str["推测相机"].c_str(), &use_conjecture_cam);

  if (imgui::Button(p_show_str["打开上一次拍屏"].c_str())) {
    p_play_balst->conjecture_ep_sc();
    FSys::open_explorer(p_play_balst->get_out_path());
  }
  imgui::SameLine();
  if (imgui::Button(p_show_str["打开文件夹"].c_str())) {
    p_play_balst->conjecture_ep_sc();
    FSys::open_explorer(p_play_balst->get_out_path().parent_path());
  }

  if (!use_conjecture_cam) {
    dear::Combo{p_show_str["选择相机"].c_str(), p_camera_path.asUTF8()} && [&]() {
      MStatus k_s;
      MItDag k_it{MItDag::kBreadthFirst, MFn::kCamera, &k_s};
      CHECK_MSTATUS_AND_RETURN(k_s, false);
      for (; !k_it.isDone(); k_it.next()) {
        MDagPath k_path;
        k_s = k_it.getPath(k_path);
        CHECK_MSTATUS_AND_RETURN(k_s, false);

        auto k_obj_tran = k_path.transform(&k_s);
        CHECK_MSTATUS_AND_RETURN(k_s, false);
        MFnDagNode k_node{k_obj_tran, &k_s};
        CHECK_MSTATUS_AND_RETURN(k_s, false);

        auto k_name = k_node.absoluteName(&k_s);
        CHECK_MSTATUS_AND_RETURN(k_s, false);
        auto k_u8        = k_name.asUTF8();
        auto k_is_select = (k_name == p_camera_path);
        if (imgui::Selectable(k_u8, k_is_select)) {
          p_camera_path = k_u8;
        }
        if (k_is_select)
          ImGui::SetItemDefaultFocus();
      }
      return true;
    };
  }

  imgui::InputText(
      p_show_str["保存路径"].c_str(),
      &p_save_path);
  return false;
}

MString comm_play_blast_maya::comm_name{"comm_play_blast_maya"};

#define doodle_filepath "-fp"
#define doodle_startTime "-st"
#define doodle_endTime "-et"
#define doodle_filepath_long "-filepath"
#define doodle_startTime_long "-startTime"
#define doodle_endTime_long "-endTime"
#define doodle_width "-width"
#define doodle_height "-height"

MStatus comm_play_blast_maya::doIt(const MArgList& in_arg) {
  MStatus k_s;
  play_blast k_p{};
  MString k_str{};
  MArgDatabase k_prase{syntax(), in_arg};

  k_str.setUTF8("开始从推测相机");
  MGlobal::displayInfo(k_str);
  k_p.conjecture_camera();

  k_str.setUTF8("开始推测集数和镜头");
  MGlobal::displayInfo(k_str);
  k_p.conjecture_ep_sc();

  if (k_prase.isFlagSet(doodle_filepath, &k_s)) {
    CHECK_MSTATUS_AND_RETURN_IT(k_s);
    auto k_path = k_prase.flagArgumentString(doodle_filepath, 0, &k_s);
    CHECK_MSTATUS_AND_RETURN_IT(k_s);
    MString k_str{};
    k_str.setUTF8("获得传入路径: ");
    k_str += k_path;
    MGlobal::displayInfo(k_str);
    k_p.set_save_path(k_path.asUTF8());
  }

  k_str.setUTF8("开始拍屏");
  MGlobal::displayInfo(k_str);

  MTime k_start_time = MAnimControl::minTime();
  if (k_prase.isFlagSet(doodle_startTime, &k_s)) {
    CHECK_MSTATUS_AND_RETURN_IT(k_s);
    k_start_time = k_prase.flagArgumentMTime(doodle_startTime, 0, &k_s);
    CHECK_MSTATUS_AND_RETURN_IT(k_s);
  }
  MTime k_end_time = MAnimControl::maxTime();
  if (k_prase.isFlagSet(doodle_endTime, &k_s)) {
    CHECK_MSTATUS_AND_RETURN_IT(k_s);
    k_end_time = k_prase.flagArgumentMTime(doodle_endTime, 0, &k_s);
    CHECK_MSTATUS_AND_RETURN_IT(k_s);
  }

  k_s = k_p.play_blast_(k_start_time, k_end_time);
  if (MGlobal::mayaState(&k_s) == MGlobal::kInteractive) {
    CHECK_MSTATUS_AND_RETURN_IT(k_s);
    FSys::open_explorer(k_p.get_out_path());
  }

  return k_s;
}
void* comm_play_blast_maya::creator() {
  return new comm_play_blast_maya{};
}
MSyntax comm_play_blast_maya::syntax() {
  MSyntax syntax{};
  syntax.addFlag(doodle_filepath, doodle_filepath_long, MSyntax::kString);
  syntax.addFlag(doodle_startTime, doodle_startTime_long, MSyntax::kTime);
  syntax.addFlag(doodle_endTime, doodle_endTime_long, MSyntax::kTime);
  // syntax.addFlag("-w", doodle_width, MSyntax::kUnsigned);
  // syntax.addFlag("-h", doodle_height, MSyntax::kUnsigned);
  return syntax;
}
#undef doodle_filepath
#undef doodle_startTime
#undef doodle_endTime
#undef doodle_filepath_long
#undef doodle_startTime_long
#undef doodle_endTime_long
#undef doodle_width
#undef doodle_height
}  // namespace doodle::maya_plug
