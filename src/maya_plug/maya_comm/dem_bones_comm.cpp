//
// Created by TD on 2022/6/30.
//

#include "dem_bones_comm.h"
#include <DemBones/DemBonesExt.h>
#include <maya/MArgDatabase.h>
#include <maya/MTime.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MAnimControl.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MComputation.h>
#include <maya/MFnMesh.h>
#include <maya/MDGModifier.h>
#include <maya/MFnIkJoint.h>

namespace doodle::maya_plug {

class dem_bones_ex : public ::Dem::DemBonesExt<std::double_t, std::double_t> {
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
  /// \brief 解算参数
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

  /// \brief 选中的物体
  syntax.setObjectType(MSyntax::MObjectFormat::kSelectionList, 1);

  syntax.enableEdit(false);
  syntax.enableQuery(false);

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
    dem.fStart.resize(2);
    dem.fStart[0]  = startFrame_p;
    dem.fStart[1]  = endFrame_p;
    /// \brief 总帧数
    dem.nF         = endFrame_p - startFrame_p;

    dem.nInitIters = nInitIters_p;
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

  /// \brief 输出结果
  // 蒙皮权重
  Eigen::MatrixXd bindWeights_p;
  // 参考输出
  Eigen::MatrixXd localRotation_p;
  // 参考转换
  Eigen::MatrixXd localTranslation_p;
  // 全局绑定矩阵输出
  Eigen::MatrixXd globalBindMatrices_p;
  // 本地旋转绑定pose
  Eigen::MatrixXd localBindPoseRotation_p;
  // 本地输出平移pose
  Eigen::MatrixXd localBindPoseTranslation_p;

  MSelectionList select_list;
  MObject mesh_obj;

  MDGModifier dg_modidier;

  std::vector<MObject> joins{};

  void init() {
    MStatus k_s;
    MItSelectionList l_it{select_list, MFn::Type::kMesh, &k_s};
    DOODLE_CHICK(k_s);
    for (; !l_it.isDone(&k_s); l_it.next()) {
      k_s = l_it.getDependNode(mesh_obj);
      DOODLE_CHICK(k_s);
    }

    MFnMesh l_mesh{mesh_obj, &k_s};
    DOODLE_CHICK(k_s);

    {  /// \brief 设置一些属性并且扩展数组大小
      dem.nV = l_mesh.numVertices(&k_s);
      DOODLE_CHICK(k_s);
      dem.v.resize(3 * dem.nF, dem.nV);
      dem.fTime.resize(dem.nF);
      dem.fv.resize(l_mesh.numPolygons(&k_s));
      DOODLE_CHICK(k_s);
      dem.subjectID.resize(dem.nF);
      dem.u.resize(dem.nS * 3, dem.nV);

      // 添加子物体的索引
      for (int s = 0; s < dem.nS; s++) {
        for (int k = dem.fStart(s); k < dem.fStart(s + 1); k++) {
          dem.subjectID(k) = s;
        }
      }
    }

    MFnDagNode l_dag_node{mesh_obj, &k_s};
    DOODLE_CHICK(k_s);
    auto l_mesh_name = l_dag_node.name(&k_s);
    DOODLE_CHICK(k_s);

    for (auto i = startFrame_p; i < endFrame_p; ++i) {
      k_s = MAnimControl::setCurrentTime(MTime{(std::double_t)i, MTime::uiUnit()});
      DOODLE_CHICK(k_s);
      DOODLE_LOG_INFO("获取网格 {} 第 {} 帧的数据", l_mesh_name, i);
      MItMeshVertex vexpoint{mesh_obj, &k_s};
      {  /// \brief 设置顶点
        dem.fTime(i) = i;
        DOODLE_CHICK(k_s);

        for (vexpoint.reset(); !vexpoint.isDone(); vexpoint.next()) {
          auto l_index = vexpoint.index();
          auto l_point = vexpoint.position(MSpace::kWorld);
          dem.v.col(l_index).segment(3 * i, 3) << l_point.x, l_point.y, l_point.z;
        }
      }
      if (i == bindFrame_p) {  /// \brief 设置绑定帧
        DOODLE_LOG_INFO("获取绑定{} 帧 网格 {} 的数据", i, l_mesh_name);
        // 设置多边形拓扑网格;
        for (vexpoint.reset(); !vexpoint.isDone(); vexpoint.next()) {
          int index  = vexpoint.index();
          MPoint pos = vexpoint.position(MSpace::kWorld);
          dem.u.col(i).segment(0, 3) << pos.x, pos.y, pos.z;
        }
        // 获得相对于polygon obj的顶点
        MItMeshPolygon polyIter{mesh_obj};
        for (polyIter.reset(); !polyIter.isDone(); polyIter.next()) {
          int index = polyIter.index();
          MIntArray vexIndexArray;
          polyIter.getVertices(vexIndexArray);

          std::vector<int> mindex{};
          mindex.resize(vexIndexArray.length());
          vexIndexArray.get(mindex.data());

          dem.fv[index] = mindex;
        }
      }
    }
  }

  void anm_compute() {
    MComputation computtation;
    computtation.beginComputation();

    DOODLE_LOG_INFO("开始计算分布骨骼......请等待");
    dem.init();
    if (computtation.isInterruptRequested()) {
      return;
    }

    DOODLE_LOG_INFO("开始计算权重......请等待");
    dem.compute();
    computtation.endComputation();

    dem.computeRTB(0,
                   localRotation_p,
                   localTranslation_p,
                   globalBindMatrices_p,
                   localBindPoseRotation_p,
                   localBindPoseTranslation_p,
                   true);
  }
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

  chick_true<doodle_error>(p_i->startFrame_p < p_i->endFrame_p,
                           DOODLE_LOC, "开始帧 {} 大于结束帧 {}",
                           p_i->startFrame_p, p_i->endFrame_p);

  if (k_prase.isFlagSet(dem_bones_comm_ns::bindFrame_f, &k_s)) {
    DOODLE_CHICK(k_s);
    MTime l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::bindFrame_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->bindFrame_p = l_value.value();
  }
  chick_true<doodle_error>(p_i->startFrame_p <= p_i->bindFrame_p &&
                               p_i->bindFrame_p < p_i->endFrame_p,
                           DOODLE_LOC, "绑定帧 {} 不在 开始帧 {} 和结束帧 {} 范围内",
                           p_i->bindFrame_p,
                           p_i->startFrame_p,
                           p_i->endFrame_p);
  if (k_prase.isFlagSet(dem_bones_comm_ns::nBones_f, &k_s)) {
    DOODLE_CHICK(k_s);
    std::uint32_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::nBones_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->nBones_p = l_value;
  }

  chick_true<doodle_error>(p_i->nBones_p > 0,
                           DOODLE_LOC, "骨骼数小于零 {}", p_i->nBones_p);

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

  k_s = k_prase.getObjects(p_i->select_list);
  DOODLE_CHICK(k_s);
  chick_true<doodle_error>(p_i->select_list.length() > 0, DOODLE_LOC, "未获得选中物体");
}
MStatus dem_bones_comm::doIt(const MArgList& in_arg) {
  get_arg(in_arg);
  p_i->set_parm();
  p_i->init();
  p_i->anm_compute();
  create_joins();
  create_anm_curve();
  create_skin();
  add_widget();
  return MStatus::kSuccess;
}
void dem_bones_comm::create_joins() {
  for (int ibone = 0; ibone < p_i->dem.nB; ibone++) {
    MObject jointObject = p_i->dg_modidier.createNode("joint");
    p_i->dg_modidier.doIt();
    MFnIkJoint joint{jointObject};
    joint.setRotationOrder(MTransformationMatrix::RotationOrder::kXYZ, true);
    p_i->joins.push_back(jointObject);
  }
}
void dem_bones_comm::create_anm_curve() {
  MFnAnimCurve aim;
  MFnIkJoint joint{};
  for (auto &&i:p_i->joins)
  {
    joint.setObject(i);
    MPlug plugtx = joint.findPlug("tx");
    MPlug plugty = joint.findPlug("ty");
    MPlug plugtz = joint.findPlug("tz");
    MPlug plugrx = joint.findPlug("rx");
    MPlug plugry = joint.findPlug("ry");
    MPlug plugrz = joint.findPlug("rz");
    //平移曲线
    MObject aimTX = aim.create(plugtx, MFnAnimCurve::AnimCurveType::kAnimCurveTL, &p_i->dg_modidier);
    MObject aimTY = aim.create(plugty, MFnAnimCurve::AnimCurveType::kAnimCurveTL, &p_i->dg_modidier);
    MObject aimTZ = aim.create(plugtz, MFnAnimCurve::AnimCurveType::kAnimCurveTL, &p_i->dg_modidier);
    //旋转曲线
    MObject aimRX = aim.create(plugrx, MFnAnimCurve::AnimCurveType::kAnimCurveTA, &p_i->dg_modidier);
    MObject aimRY = aim.create(plugry, MFnAnimCurve::AnimCurveType::kAnimCurveTA, &p_i->dg_modidier);
    MObject aimRZ = aim.create(plugrz, MFnAnimCurve::AnimCurveType::kAnimCurveTA, &p_i->dg_modidier);

  }
}
void dem_bones_comm::create_skin() {
}
void dem_bones_comm::add_widget() {
}

dem_bones_comm::~dem_bones_comm() = default;

}  // namespace doodle::maya_plug
