//
// Created by TD on 2022/7/4.
//

#include "dem_cloth_to_fbx.h"

#include <doodle_app/lib_warp/imgui_warp.h>

#include <maya/MAnimControl.h>
#include <maya/MGlobal.h>

namespace doodle::maya_plug {
class dem_cloth_to_fbx::impl {
 public:
  std::int32_t startFrame_p{0};
  std::int32_t endFrame_p{120};
  std::int32_t bindFrame_p{0};
  std::int32_t nBones_p{180};
  std::int32_t nIters_p{50};
  std::int32_t nInitIters_p{25};
  std::int32_t nTransIters_p{10};
  std::double_t transAffine_p{10};
  std::double_t transAffineNorm_p{4};
  std::int32_t nWeightsIters_p{8};
  std::int32_t nonZeroWeightsNum_p{8};
  std::double_t weightsSmooth_p{1e-4};
  std::double_t weightsSmoothStep_p{1};
  std::int32_t isBindUpdate_p{0};
  std::string title_name_{};
  bool open{true};
  void run() {
    auto l_py = fmt::format(
        R"(
import maya.cmds as cmds

select_list = cmds.ls(sl=True)
if select_list:
    for obj in select_list:
        j_list = cmds.doodle_comm_dem_bones(obj,
                                            sf={0},ef={1},bf={2},nb={3},nit={4},nii={5},nti={6},nwi={7})
        cmds.currentTime({2})
        l_du = cmds.duplicate(obj, rr=True)
        j_list.append(l_du[0])
        cmds.skinCluster(j_list)
        cmds.doodle_comm_dem_bones_weiget(l_du)
)",
        startFrame_p, endFrame_p, startFrame_p, nBones_p, nIters_p, nInitIters_p, nTransIters_p, nWeightsIters_p
    );

    MGlobal::executePythonCommandOnIdle(d_str{l_py}, true);
  }
};
dem_cloth_to_fbx::dem_cloth_to_fbx() : p_i(std::make_unique<impl>()) {
  this->p_i->title_name_ = std::string{name};
  init();
}
void dem_cloth_to_fbx::init() {
  p_i->startFrame_p = MAnimControl::minTime().value();
  p_i->endFrame_p   = MAnimControl::maxTime().value();
}
bool dem_cloth_to_fbx::render() {
  ImGui::InputInt("开始帧", &p_i->startFrame_p);
  ImGui::InputInt("结束帧", &p_i->endFrame_p);

  ImGui::InputInt("骨骼数", &p_i->nBones_p);
  ImGui::InputInt("全局迭代数", &p_i->nIters_p);
  ImGui::InputInt("初始化迭代数", &p_i->nInitIters_p);
  ImGui::InputInt("骨骼迭代数", &p_i->nTransIters_p);
  ImGui::InputInt("权重迭代数", &p_i->nWeightsIters_p);

  if (ImGui::Button("转换")) {
    p_i->run();
  }
  return p_i->open;
}
const std::string& dem_cloth_to_fbx::title() const { return p_i->title_name_; }

dem_cloth_to_fbx::~dem_cloth_to_fbx() = default;
}  // namespace doodle::maya_plug
