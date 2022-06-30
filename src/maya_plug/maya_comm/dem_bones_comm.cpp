//
// Created by TD on 2022/6/30.
//

#include "dem_bones_comm.h"
#include <DemBones/DemBonesExt.h>
#include <maya/MArgDatabase.h>
#include <maya/MTime.h>

namespace doodle::maya_plug {

class dem_bones_ex : public ::Dem::DemBonesExt<std::double_t, std::float_t> {
 public:
  dem_bones_ex()  = default;
  ~dem_bones_ex() = default;
  // 初始化中每次分割骨簇之前调用回调函数。
  void cbInitSplitBegin() override{};
  // 初始化中每次对骨簇进行分割后都会调用回调函数
  void cbInitSplitEnd() override{};
  // 在每次全局迭代更新之前调用回调函数
  void cbIterBegin() override{};
  // 在每次全局迭代更新后调用回调函数，如果返回true，则停止迭代。
  bool cbIterEnd() override { return false; };
  // 在每个外观权重更新之前调用的回调函数
  void cbWeightsBegin() override{};
  // 每次蒙皮权重更新后调用的回调函数
  void cbWeightsEnd() override{};
  // 在每次骨骼转换更新之前调用的回调函数
  void cbTranformationsBegin() override{};
  // 每次骨骼转换更新后调用的回调函数
  void cbTransformationsEnd() override{};

  // 每个局部骨骼转换更新迭代后调用的回调函数，如果返回true，则停止迭代
  void cbWeightsIterBegin() override{};
  //	在每个局部权重更新迭代后调用的回调函数，如果返回true，则停止迭代。
  bool cbWeightsIterEnd() override { return false; };
};

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
  syntax.addFlag(isBindUpdate_f, isBindUpdate_lf, MSyntax::kUnsigned);
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
  dem_bones_ex dem;
  void set_parm() {
    dem.nB                = nBones_p;
    dem.nIters            = nIters_p;
    dem.nTransIters       = nTransIters_p;
    dem.nWeightsIters     = nWeightsIters_p;
    dem.bindUpdate        = isBindUpdate_p;
    dem.transAffine       = transAffine_p;
    dem.transAffineNorm   = transAffineNorm_p;
    dem.nnz               = nonZeroWeightsNum_p;
    dem.weightsSmooth     = weightsSmooth_p;
    dem.weightsSmoothStep = weightsSmoothStep_p;

    dem.nS                = 1;
    /// \brief 开始帧结束帧
    dem.fStart[0]         = startFrame_p;
    dem.fStart[1]         = endFrame_p;
    /// \brief 总帧数
    dem.nF                = endFrame_p - startFrame_p;

    dem.nInitIters        = nInitIters_p;
  }

  std::int32_t startFrame_p{0};
  std::int32_t endFrame_p{120};
  std::int32_t bindFrame_p{0};
  std::int32_t nBones_p{1};
  std::int32_t nInitIters_p{10};
  std::int32_t nIters_p{30};
  std::int32_t nTransIters_p{1};
  std::int32_t isBindUpdate_p{};
  std::double_t transAffine_p{10};
  std::double_t transAffineNorm_p{4};
  std::int32_t nWeightsIters_p{3};
  std::int32_t nonZeroWeightsNum_p{8};
  std::double_t weightsSmooth_p{1e-4};
  std::double_t weightsSmoothStep_p{1};
};
dem_bones_comm::dem_bones_comm()
    : p_i(std::make_unique<impl>()) {
}
void dem_bones_comm::get_arg(const MArgList& in_arg) {
  MStatus k_s;
  MArgDatabase k_prase{syntax(), in_arg};

  if (k_prase.isFlagSet(dem_bones_comm_ns::startFrame_f, &k_s)) {
    DOODLE_CHICK(k_s);
    MTime l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::startFrame_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->startFrame_p = l_value.value();
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::endFrame_f, &k_s)) {
    DOODLE_CHICK(k_s);
    MTime l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::endFrame_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->endFrame_p = l_value.value();
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::bindFrame_f, &k_s)) {
    DOODLE_CHICK(k_s);
    MTime l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::bindFrame_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->bindFrame_p = l_value.value();
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::nBones_f, &k_s)) {
    DOODLE_CHICK(k_s);
    std::uint32_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::nBones_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->nBones_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::nInitIters_f, &k_s)) {
    DOODLE_CHICK(k_s);
    std::uint32_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::nInitIters_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->nInitIters_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::nIters_f, &k_s)) {
    DOODLE_CHICK(k_s);
    std::uint32_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::nIters_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->nIters_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::nTransIters_f, &k_s)) {
    DOODLE_CHICK(k_s);
    std::uint32_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::nTransIters_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->nTransIters_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::isBindUpdate_f, &k_s)) {
    DOODLE_CHICK(k_s);
    std::uint32_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::isBindUpdate_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->isBindUpdate_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::transAffine_f, &k_s)) {
    DOODLE_CHICK(k_s);
    std::double_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::transAffine_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->transAffine_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::transAffineNorm_f, &k_s)) {
    DOODLE_CHICK(k_s);
    std::double_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::transAffineNorm_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->transAffineNorm_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::nWeightsIters_f, &k_s)) {
    DOODLE_CHICK(k_s);
    std::uint32_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::nWeightsIters_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->nWeightsIters_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::nonZeroWeightsNum_f, &k_s)) {
    DOODLE_CHICK(k_s);
    std::uint32_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::nonZeroWeightsNum_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->nonZeroWeightsNum_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::weightsSmooth_f, &k_s)) {
    DOODLE_CHICK(k_s);
    std::double_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::weightsSmooth_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->weightsSmooth_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::weightsSmoothStep_f, &k_s)) {
    DOODLE_CHICK(k_s);
    std::double_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::weightsSmoothStep_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->weightsSmoothStep_p = l_value;
  }
}
MStatus dem_bones_comm::doIt(const MArgList& in_arg) {
  get_arg(in_arg);
  p_i->set_parm();
  return TemplateAction::doIt(in_arg);
}

dem_bones_comm::~dem_bones_comm() = default;

}  // namespace doodle::maya_plug
