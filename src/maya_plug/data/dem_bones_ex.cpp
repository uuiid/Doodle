//
// Created by TD on 2022/7/1.
//
#include "dem_bones_ex.h"

namespace doodle::maya_plug {

void dem_bones_ex::cbInitSplitBegin() { DemBones::cbInitSplitBegin(); }
void dem_bones_ex::cbInitSplitEnd() { DemBones::cbInitSplitEnd(); }
void dem_bones_ex::cbIterBegin() { DemBones::cbIterBegin(); }
bool dem_bones_ex::cbIterEnd() { return false; }
void dem_bones_ex::cbWeightsBegin() { DemBones::cbWeightsBegin(); }
void dem_bones_ex::cbWeightsEnd() { DemBones::cbWeightsEnd(); }
void dem_bones_ex::cbTranformationsBegin() { DemBones::cbTranformationsBegin(); }
void dem_bones_ex::cbTransformationsEnd() { DemBones::cbTransformationsEnd(); }
void dem_bones_ex::cbWeightsIterBegin() { DemBones::cbWeightsIterBegin(); }
bool dem_bones_ex::cbWeightsIterEnd() { return false; }
}  // namespace doodle::maya_plug
