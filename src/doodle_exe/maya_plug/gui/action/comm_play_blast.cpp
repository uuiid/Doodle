//
// Created by TD on 2021/11/16.
//

#include "comm_play_blast.h"

#include "data/play_blast.h"
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/lib_warp/imgui_warp.h>

#include <maya/MAnimControl.h>
#include <maya/MDrawContext.h>
#include <maya/MFileIO.h>
#include <maya/MFnCamera.h>
#include <maya/MItDag.h>

#include <maya_plug/data/create_hud_node.h>

namespace doodle::maya_plug {

// comm_play_blast::comm_play_blast()
//     : command_base(),
//       use_conjecture_cam(true),
//       p_save_path(core_set::getSet().get_cache_root("maya_play_blast").generic_string()),
//       p_play_balst(new_object<play_blast>()) {
//   p_show_str = make_imgui_name(this,
//                                "保存路径",
//                                "拍摄",
//                                "拍屏",
//                                "hud",
//                                "选择相机",
//                                "推测相机",
//                                "打开文件夹",
//                                "打开上一次拍屏");
// }

// bool comm_play_blast::render() {
//   if (imgui::Button(p_show_str["拍屏"].c_str())) {

//     if (p_play_balst->conjecture_ep_sc()) {
//       p_play_balst->set_save_dir(p_save_path);
//       p_play_balst->play_blast_(MAnimControl::minTime(), MAnimControl::maxTime());
//     } else {
//       DOODLE_LOG_ERROR("无法分析路径得到镜头号和集数， 请重新设置文件路径");
//     }
//   }
//   if (imgui::Button(p_show_str["hud"].c_str())) {
//     create_hud_node k_c{};
//     k_c();
//   }

//   imgui::Checkbox(p_show_str["推测相机"].c_str(), &use_conjecture_cam);

//   if (imgui::Button(p_show_str["打开上一次拍屏"].c_str())) {
//     p_play_balst->conjecture_ep_sc();
//     FSys::open_explorer(p_play_balst->get_out_path());
//   }
//   imgui::SameLine();
//   if (imgui::Button(p_show_str["打开文件夹"].c_str())) {
//     p_play_balst->conjecture_ep_sc();
//     FSys::open_explorer(p_play_balst->get_out_path().parent_path());
//   }

//   if (!use_conjecture_cam) {
//     dear::Combo{p_show_str["选择相机"].c_str(), p_camera_path.asUTF8()} && [&]() {
//       MStatus k_s;
//       MItDag k_it{MItDag::kBreadthFirst, MFn::kCamera, &k_s};
//       CHECK_MSTATUS_AND_RETURN(k_s, false);
//       for (; !k_it.isDone(); k_it.next()) {
//         MDagPath k_path;
//         k_s = k_it.getPath(k_path);
//         CHECK_MSTATUS_AND_RETURN(k_s, false);

//         auto k_obj_tran = k_path.transform(&k_s);
//         CHECK_MSTATUS_AND_RETURN(k_s, false);
//         MFnDagNode k_node{k_obj_tran, &k_s};
//         CHECK_MSTATUS_AND_RETURN(k_s, false);

//         auto k_name = k_node.absoluteName(&k_s);
//         CHECK_MSTATUS_AND_RETURN(k_s, false);
//         auto k_u8        = k_name.asUTF8();
//         auto k_is_select = (k_name == p_camera_path);
//         if (imgui::Selectable(k_u8, k_is_select)) {
//           p_camera_path = k_u8;
//         }
//         if (k_is_select)
//           ImGui::SetItemDefaultFocus();
//       }
//       return true;
//     };
//   }

//   imgui::InputText(
//       p_show_str["保存路径"].c_str(),
//       &p_save_path);
//   return false;
// }

}  // namespace doodle::maya_plug
