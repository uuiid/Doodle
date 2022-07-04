//
// Created by TD on 2022/7/4.
//

#include "dem_cloth_to_fbx.h"

#include <doodle_lib/lib_warp/imgui_warp.h>
#include <maya/MAnimControl.h>
#include <maya/MGlobal.h>

namespace doodle::maya_plug {
class dem_cloth_to_fbx::impl {
 public:
  std::int32_t startFrame_p{0};
  std::int32_t endFrame_p{120};
  std::int32_t bindFrame_p{0};
  std::int32_t nBones_p{30};
  std::int32_t nIters_p{30};
  std::int32_t nInitIters_p{10};
  std::int32_t nTransIters_p{5};
  std::double_t transAffine_p{10};
  std::double_t transAffineNorm_p{4};
  std::int32_t nWeightsIters_p{3};
  std::int32_t nonZeroWeightsNum_p{8};
  std::double_t weightsSmooth_p{1e-4};
  std::double_t weightsSmoothStep_p{1};
  std::int32_t isBindUpdate_p{0};

  void run() {
    auto l_py = fmt::format(R"(
import maya.cmds as cmds

select_list = cmds.ls(sl=True)
if select_list:
    j_list = cmds.doodle_comm_dem_bones(select_list[0],
                                        sf={0},ef={1},bf={2},nb={3},nit={4},niit={5},nti={6})
    cmds.currentTime({2})
    l_du = cmds.duplicate(select_list[0], rr=True)
    j_list.append(l_du[0])
    cmds.skinCluster(j_list)
    cmds.doodle_comm_dem_bones_weiget(l_du)
)",
                            startFrame_p,
                            endFrame_p,
                            startFrame_p,
                            nBones_p,
                            nIters_p,
                            nInitIters_p,
                            nTransIters_p);

    MGlobal::executePythonCommandOnIdle(d_str{l_py},
                                        true);
  }
};
dem_cloth_to_fbx::dem_cloth_to_fbx()
    : p_i(std::make_unique<impl>()) {
}
void dem_cloth_to_fbx::init() {
  window_panel::init();
  p_i->startFrame_p = MAnimControl::minTime().value();
  p_i->endFrame_p   = MAnimControl::maxTime().value();
}
void dem_cloth_to_fbx::render() {
  ImGui::InputInt("开始帧", &p_i->startFrame_p);
  ImGui::InputInt("结束帧", &p_i->startFrame_p);

  ImGui::SliderInt("骨骼数", &p_i->nBones_p, 0, 300);
  ImGui::SliderInt("全局迭代数", &p_i->nIters_p, 0, 300);
  ImGui::SliderInt("初始化迭代数", &p_i->nInitIters_p, 0, 300);
  ImGui::SliderInt("骨骼迭代数", &p_i->nTransIters_p, 0, 300);

  if (ImGui::Button("转换")) {
    p_i->run();
  }
}

dem_cloth_to_fbx::~dem_cloth_to_fbx() = default;
}  // namespace doodle::maya_plug
