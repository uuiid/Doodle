//
// Created by TD on 2022/6/30.
//

#include "dem_bones_comm.h"

#include <maya/MArgParser.h>
namespace doodle::maya_plug {
namespace dem_bones_comm_ns {

constexpr char startFrame_f[]         = "sf";
constexpr char startFrame_lf[]        = "startFrame";
constexpr char endFrame_f[]           = "ef";
constexpr char endFrame_lf[]          = "endFrame";
constexpr char bindFrame_f[]          = "bf";
constexpr char bindFrame_lf[]         = "bindFrame";
constexpr char nBones_f[]             = "nb";
constexpr char nBones_lf[]            = "nBones";
constexpr char nInitIters_f[]         = "niit";
constexpr char nInitIters_lf[]        = "nInitIters";
constexpr char nIters_f[]             = "nit";
constexpr char nIters_lf[]            = "nIters";
constexpr char nTransIters_f[]        = "nti";
constexpr char nTransIters_lf[]       = "nTransIters";
constexpr char isBindUpdate_f[]       = "bup";
constexpr char isBindUpdate_lf[]      = "isBindUpdate";
constexpr char transAffine_f[]        = "ta";
constexpr char transAffine_lf[]       = "transAffine";
constexpr char transAffineNorm_f[]    = "tan";
constexpr char transAffineNorm_lf[]   = "transAffineNorm";
constexpr char nWeightsIters_f[]      = "nwi";
constexpr char nWeightsIters_lf[]     = "nWeightsIters";
constexpr char nonZeroWeightsNum_f[]  = "nzwn";
constexpr char nonZeroWeightsNum_lf[] = "nonZeroWeightsNum";
constexpr char weightsSmooth_f[]      = "ws";
constexpr char weightsSmooth_lf[]     = "weightsSmooth";
constexpr char weightsSmoothStep_f[]  = "wss";
constexpr char weightsSmoothStep_lf[] = "weightsSmoothStep";
MSyntax syntax() {
  MSyntax syntax{};
  syntax.addFlag(startFrame_f, startFrame_lf, MSyntax::kTime);
  syntax.addFlag(endFrame_f, endFrame_lf, MSyntax::kTime);
  syntax.addFlag(bindFrame_f, bindFrame_lf, MSyntax::kTime);
  syntax.addFlag(nBones_f, nBones_lf, MSyntax::kUnsigned);
  syntax.addFlag(nInitIters_f, nInitIters_lf, MSyntax::kUnsigned);
  syntax.addFlag(nIters_f, nIters_lf, MSyntax::kUnsigned);
  syntax.addFlag(nTransIters_f, nTransIters_lf, MSyntax::kUnsigned);
  syntax.addFlag(isBindUpdate_f, isBindUpdate_lf, MSyntax::kBoolean);
  syntax.addFlag(transAffine_f, transAffine_lf, MSyntax::kDouble);
  syntax.addFlag(transAffineNorm_f, transAffineNorm_lf, MSyntax::kDouble);
  syntax.addFlag(nWeightsIters_f, nWeightsIters_lf, MSyntax::kUnsigned);
  syntax.addFlag(nonZeroWeightsNum_f, nonZeroWeightsNum_lf, MSyntax::kUnsigned);
  syntax.addFlag(weightsSmooth_f, weightsSmooth_lf, MSyntax::kDouble);
  syntax.addFlag(weightsSmoothStep_f, weightsSmoothStep_lf, MSyntax::kDouble);
  return syntax;
}
}  // namespace dem_bones_comm_ns
class dem_bones_comm::impl {
 public:
  std::int32_t startFrame_p;
  std::int32_t endFrame_p;
  std::int32_t bindFrame_p;
  std::int32_t nBones_p;
  std::int32_t nInitIters_p;
  std::int32_t nIters_p;
  std::int32_t nTransIters_p;
  std::int32_t isBindUpdate_p;
  std::double_t transAffine_p;
  std::double_t transAffineNorm_p;
  std::int32_t nWeightsIters_p;
  std::int32_t nonZeroWeightsNum_p;
  std::double_t weightsSmooth_p;
  std::double_t weightsSmoothStep_p;
};
dem_bones_comm::dem_bones_comm()
    : p_i(std::make_unique<impl>()) {
}
void dem_bones_comm::get_arg(const MArgList& in_arg) {
  MStatus k_s;
  MArgParser k_prase{syntax(), in_arg};

  if (k_prase.isFlagSet(dem_bones_comm_ns::startFrame_f, &k_s)) {
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::endFrame_f, &k_s)) {
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::bindFrame_f, &k_s)) {
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::nBones_f, &k_s)) {
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::nInitIters_f, &k_s)) {
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::nIters_f, &k_s)) {
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::nTransIters_f, &k_s)) {
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::isBindUpdate_f, &k_s)) {
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::transAffine_f, &k_s)) {
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::transAffineNorm_f, &k_s)) {
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::nWeightsIters_f, &k_s)) {
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::nonZeroWeightsNum_f, &k_s)) {
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::weightsSmooth_f, &k_s)) {
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::weightsSmoothStep_f, &k_s)) {
  }
}
MStatus dem_bones_comm::doIt(const MArgList& in_arg) {
  get_arg(in_arg);
  return TemplateAction::doIt(in_arg);
}

dem_bones_comm::~dem_bones_comm() = default;

}  // namespace doodle::maya_plug
