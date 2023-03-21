//
// Created by TD on 2022/6/30.
//

#include "dem_bones_comm.h"

#include <doodle_core/lib_warp/entt_warp.h>

#include <maya_plug/data/dem_bones_ex.h>
#include <maya_plug/data/maya_tool.h>

#include <DemBones/DemBonesExt.h>
#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MComputation.h>
#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MDoubleArray.h>
#include <maya/MEulerRotation.h>
#include <maya/MFnIkJoint.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItSelectionList.h>
#include <maya/MMatrix.h>
#include <maya/MQuaternion.h>
#include <maya/MSelectionList.h>
#include <maya/MTime.h>
#include <maya/MTransformationMatrix.h>

namespace doodle::maya_plug {

namespace dem_bones_comm_ns {

constexpr char startFrame_f[]         = "-sf";
constexpr char startFrame_lf[]        = "-startFrame";
constexpr char endFrame_f[]           = "-ef";
constexpr char endFrame_lf[]          = "-endFrame";
constexpr char bindFrame_f[]          = "-bf";
constexpr char bindFrame_lf[]         = "-bindFrame";
constexpr char nBones_f[]             = "-nb";
constexpr char nBones_lf[]            = "-nBones";
constexpr char nInitIters_f[]         = "-nii";
constexpr char nInitIters_lf[]        = "-nInitIters";
constexpr char nIters_f[]             = "-nit";
constexpr char nIters_lf[]            = "-nIters";
constexpr char nTransIters_f[]        = "-nti";
constexpr char nTransIters_lf[]       = "-nTransIters";
constexpr char isBindUpdate_f[]       = "-bup";
constexpr char isBindUpdate_lf[]      = "-isBindUpdate";
constexpr char transAffine_f[]        = "-ta";
constexpr char transAffine_lf[]       = "-transAffine";
constexpr char transAffineNorm_f[]    = "-tan";
constexpr char transAffineNorm_lf[]   = "-transAffineNorm";
constexpr char nWeightsIters_f[]      = "-nwi";
constexpr char nWeightsIters_lf[]     = "-nWeightsIters";
constexpr char nonZeroWeightsNum_f[]  = "-nzw";
constexpr char nonZeroWeightsNum_lf[] = "-nonZeroWeightsNum";
constexpr char weightsSmooth_f[]      = "-ws";
constexpr char weightsSmooth_lf[]     = "-weightsSmooth";
constexpr char weightsSmoothStep_f[]  = "-wss";
constexpr char weightsSmoothStep_lf[] = "-weightsSmoothStep";
constexpr char parent_f[]             = "-p";
constexpr char parent_lf[]            = "-parent";

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
  syntax.addFlag(parent_f, parent_lf, MSyntax::kString);

  /// \brief 选中的物体
  syntax.setObjectType(MSyntax::MObjectFormat::kSelectionList, 1);

  syntax.enableEdit(false);
  syntax.enableQuery(false);

  return syntax;
}
}  // namespace dem_bones_comm_ns
class dem_bones_comm::impl {
 public:
  impl() : dem(g_reg()->ctx().emplace<dem_bones_ex>()) {}
  dem_bones_ex& dem;
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
    dem.weightEps         = 1e-15;

    dem.nS                = 1;
    dem.nF                = endFrame_p - startFrame_p;
    /// \brief 开始帧结束帧
    dem.fStart.resize(2);
    dem.fStart[0]  = 0;
    dem.fStart[1]  = dem.fStart[0] + dem.nF;
    /// \brief 总帧数

    dem.nInitIters = nInitIters_p;
  }

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
  MObject skin_mesh_obj;
  MObject skin_obj;

  MObject parent_tran;

  MDagModifier dg_modidier;

  std::vector<MObject> joins{};

  std::vector<MMatrix> tran_inverse_list;

  void push_time_tran_inverse() {
    if (parent_tran.isNull()) return;
    MStatus k_s{};
    MFnTransform l_fn_tran{};
    k_s = l_fn_tran.setObject(parent_tran);
    DOODLE_MAYA_CHICK(k_s);
    auto l_t = l_fn_tran.transformation(&k_s);
    DOODLE_MAYA_CHICK(k_s);

    tran_inverse_list.push_back(l_t.asMatrixInverse());
  }

  void init() {
    MStatus k_s;
    MItSelectionList l_it{select_list, MFn::Type::kMesh, &k_s};
    DOODLE_MAYA_CHICK(k_s);
    for (; !l_it.isDone(&k_s); l_it.next()) {
      k_s = l_it.getDependNode(mesh_obj);
      DOODLE_MAYA_CHICK(k_s);
    }

    MFnMesh l_mesh{mesh_obj, &k_s};
    DOODLE_MAYA_CHICK(k_s);

    {  /// \brief 设置一些属性并且扩展数组大小
      dem.nV = l_mesh.numVertices(&k_s);
      DOODLE_MAYA_CHICK(k_s);
      dem.v.resize(3 * dem.nF, dem.nV);
      dem.fTime.resize(dem.nF);
      dem.fv.resize(l_mesh.numPolygons(&k_s));
      DOODLE_MAYA_CHICK(k_s);
      dem.subjectID.resize(dem.nF);
      dem.u.resize(3 * dem.nS, dem.nV);
      //      dem.preMulInv.resize(4 * dem.nS, 4 * dem.nB);

      // 添加子物体的索引
      for (int s = 0; s < dem.nS; s++) {
        for (int k = dem.fStart(s); k < dem.fStart(s + 1); k++) {
          dem.subjectID(k) = s;
        }
      }
    }
    // 如果有父物体需要添加转换矩阵

    MFnDagNode l_dag_node{mesh_obj, &k_s};
    DOODLE_MAYA_CHICK(k_s);
    auto l_mesh_name = l_dag_node.name(&k_s);
    DOODLE_MAYA_CHICK(k_s);
    tran_inverse_list.clear();

    for (auto i = 0; i < dem.nF; ++i) {
      k_s = MGlobal::viewFrame((std::double_t)(i + startFrame_p));
      DOODLE_MAYA_CHICK(k_s);
      DOODLE_LOG_INFO("获取网格 {} 第 {} 帧的数据", l_mesh_name, i);
      /// \brief 添加当前帧的逆矩阵
      push_time_tran_inverse();

      MItMeshVertex vexpoint{mesh_obj, &k_s};
      DOODLE_MAYA_CHICK(k_s);
      /// \brief 设置顶点
      dem.fTime(i) = i;

      for (vexpoint.reset(); !vexpoint.isDone(); vexpoint.next()) {
        auto l_index = vexpoint.index();
        auto l_point = vexpoint.position(MSpace::kWorld);
        dem.v.col(l_index).segment(3 * i, 3) << l_point.x, l_point.y, l_point.z;
      }

      if (i + startFrame_p == bindFrame_p) {  /// \brief 设置绑定帧
        DOODLE_LOG_INFO("获取绑定{} 帧 网格 {} 的数据", i, l_mesh_name);
        // 设置多边形拓扑网格;
        for (vexpoint.reset(); !vexpoint.isDone(); vexpoint.next()) {
          int index  = vexpoint.index();
          MPoint pos = vexpoint.position(MSpace::kWorld);
          dem.u.col(index).segment(0, 3) << pos.x, pos.y, pos.z;
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
          //          for (unsigned int vexindex = 0; vexindex < vexIndexArray.length(); vexindex++) {
          //            mindex.push_back(vexIndexArray[vexindex]);
          //          }
          dem.fv[index] = mindex;
        }
      }
    }
  }

  void anm_compute() {
    MComputation computtation;
    computtation.beginComputation();

    DOODLE_LOG_INFO("开始计算分布骨骼......请等待");
    if (computtation.isInterruptRequested()) {
      return;
    }

    DOODLE_LOG_INFO("开始计算权重......请等待");
    dem.compute();
    computtation.endComputation();

    dem.computeRTB(
        0, localRotation_p, localTranslation_p, globalBindMatrices_p, localBindPoseRotation_p,
        localBindPoseTranslation_p, false
    );
  }
};

dem_bones_comm::dem_bones_comm() : p_i() {
  g_reg()->ctx().erase<dem_bones_ex>();
  p_i = std::make_unique<impl>();
}
void dem_bones_comm::get_arg(const MArgList& in_arg) {
  MStatus k_s;
  MArgDatabase k_prase{syntax(), in_arg};

  if (k_prase.isFlagSet(dem_bones_comm_ns::startFrame_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    MTime l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::startFrame_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->startFrame_p = l_value.value();
  } else {
    p_i->startFrame_p = MAnimControl::minTime().value();
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::endFrame_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    MTime l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::endFrame_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->endFrame_p = l_value.value();
  } else {
    p_i->endFrame_p = MAnimControl::maxTime().value();
  }

  p_i->startFrame_p < p_i->endFrame_p
      ? void()
      : throw_exception(doodle_error{"开始帧 {} 大于结束帧 {}"s, p_i->startFrame_p, p_i->endFrame_p});

  if (k_prase.isFlagSet(dem_bones_comm_ns::bindFrame_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    MTime l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::bindFrame_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->bindFrame_p = l_value.value();
  }

  p_i->startFrame_p <= p_i->bindFrame_p && p_i->bindFrame_p < p_i->endFrame_p
      ? void()
      : throw_exception(doodle_error{
            "绑定帧 {} 不在 开始帧 {} 和结束帧 {} 范围内"s, p_i->bindFrame_p, p_i->startFrame_p, p_i->endFrame_p});

  if (k_prase.isFlagSet(dem_bones_comm_ns::nBones_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    std::uint32_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::nBones_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->nBones_p = l_value;
  }

  p_i->nBones_p > 0 ? void() : throw_exception(doodle_error{"骨骼数小于零 {}"s, p_i->nBones_p});
  if (k_prase.isFlagSet(dem_bones_comm_ns::nInitIters_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    std::uint32_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::nInitIters_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->nInitIters_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::nIters_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    std::uint32_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::nIters_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->nIters_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::nTransIters_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    std::uint32_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::nTransIters_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->nTransIters_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::isBindUpdate_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    std::uint32_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::isBindUpdate_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->isBindUpdate_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::transAffine_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    std::double_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::transAffine_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->transAffine_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::transAffineNorm_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    std::double_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::transAffineNorm_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->transAffineNorm_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::nWeightsIters_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    std::uint32_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::nWeightsIters_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->nWeightsIters_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::nonZeroWeightsNum_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    std::uint32_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::nonZeroWeightsNum_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->nonZeroWeightsNum_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::weightsSmooth_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    std::double_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::weightsSmooth_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->weightsSmooth_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::weightsSmoothStep_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    std::double_t l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::weightsSmoothStep_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->weightsSmoothStep_p = l_value;
  }
  if (k_prase.isFlagSet(dem_bones_comm_ns::parent_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    MString l_value{};
    k_s = k_prase.getFlagArgument(dem_bones_comm_ns::parent_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    MSelectionList l_select{};
    k_s = l_select.add(l_value);
    DOODLE_MAYA_CHICK(k_s);
    MDagPath l_path;

    k_s = l_select.getDagPath(0, l_path);
    DOODLE_MAYA_CHICK(k_s);

    p_i->parent_tran = l_path.transform(&k_s);
    DOODLE_MAYA_CHICK(k_s);
  }

  k_s = k_prase.getObjects(p_i->select_list);
  DOODLE_MAYA_CHICK(k_s);
  DOODLE_CHICK(p_i->select_list.length() > 0, doodle_error{"未获得选中物体"s});
}
MStatus dem_bones_comm::doIt(const MArgList& in_arg) {
  get_arg(in_arg);
  p_i->set_parm();
  p_i->init();
  p_i->anm_compute();
  create_joins();
  create_anm_curve();
  set_result();

  return MStatus::kSuccess;
}
void dem_bones_comm::create_joins() {
  MStatus k_s{};

  for (int ibone = 0; ibone < p_i->dem.nB; ibone++) {
    MFnIkJoint joint{};
    auto l_joint_obj = joint.create(p_i->parent_tran, &k_s);
    DOODLE_MAYA_CHICK(k_s);
    k_s = joint.setRotationOrder(MTransformationMatrix::RotationOrder::kXYZ, true);
    DOODLE_MAYA_CHICK(k_s);
    p_i->joins.push_back(l_joint_obj);
  }
  p_i->dem.joins = p_i->joins;
}
void dem_bones_comm::create_anm_curve() {
  MStatus k_s{};
  MFnAnimCurve aim{};
  MFnIkJoint joint{};

  for (auto l_b = 0; l_b < p_i->dem.nB; ++l_b) {
    joint.setObject(p_i->joins[l_b]);
    MPlug plugtx = joint.findPlug("tx");
    MPlug plugty = joint.findPlug("ty");
    MPlug plugtz = joint.findPlug("tz");
    MPlug plugrx = joint.findPlug("rx");
    MPlug plugry = joint.findPlug("ry");
    MPlug plugrz = joint.findPlug("rz");
    MTimeArray l_time{};
#define DOODLE_ADD_ANM_declaration(axis) \
  MDoubleArray l_value_tran_##axis{};    \
  MDoubleArray l_value_rot_##axis{};

    DOODLE_ADD_ANM_declaration(x);
    DOODLE_ADD_ANM_declaration(y);
    DOODLE_ADD_ANM_declaration(z);

#define DOODLE_ADD_ANM_set(axis)             \
  l_value_tran_##axis.append(l_tran.axis()); \
  l_value_rot_##axis.append(l_erot.axis);

    for (int l_f = 0; l_f < p_i->dem.nF; ++l_f) {
      auto l_tran = p_i->localTranslation_p.col(l_b).segment<3>(3 * l_f);
      auto l_rot  = p_i->localRotation_p.col(l_b).segment<3>(3 * l_f);
      MEulerRotation l_erot{l_rot.x(), l_rot.y(), l_rot.z(), MEulerRotation::kXYZ};
      auto l_qrot = l_erot.asQuaternion();
      l_erot      = l_qrot.asEulerRotation();
      //      MTransformationMatrix l_tran_mat = joint.transformation();
      //      k_s                              = l_tran_mat.setTranslation(MVector{l_tran.x(), l_tran.y(), l_tran.z()},
      //      MSpace::Space::kWorld); DOODLE_MAYA_CHICK(k_s); l_tran_mat.setRotationOrientation(l_qrot);
      //      DOODLE_MAYA_CHICK(k_s);
      //      if (!p_i->tran_inverse_list.empty()) {
      //        auto l_matrix = l_tran_mat.asMatrix() * p_i->tran_inverse_list[l_f];
      //        l_tran_mat    = l_matrix;
      //      }
      //      l_erot          = l_tran_mat.eulerRotation();
      //      auto l_vex_tran = l_tran_mat.getTranslation(MSpace::Space::kTransform, &k_s);
      //      DOODLE_MAYA_CHICK(k_s);

      l_time.append(MTime{(std::double_t)l_f + p_i->startFrame_p, MTime::uiUnit()});
      DOODLE_ADD_ANM_set(x);
      DOODLE_ADD_ANM_set(y);
      DOODLE_ADD_ANM_set(z);
    }
#define DOODLE_ADD_ANM_set_anm(axis)                                                     \
  aim.create(plugt##axis, MFnAnimCurve::AnimCurveType::kAnimCurveTL, &p_i->dg_modidier); \
  k_s = aim.addKeys(&l_time, &l_value_tran_##axis);                                      \
  DOODLE_MAYA_CHICK(k_s);                                                                \
  aim.create(plugr##axis, MFnAnimCurve::AnimCurveType::kAnimCurveTA, &p_i->dg_modidier); \
  k_s = aim.addKeys(&l_time, &l_value_rot_##axis);                                       \
  DOODLE_MAYA_CHICK(k_s);

    DOODLE_ADD_ANM_set_anm(x);
    DOODLE_ADD_ANM_set_anm(y);
    DOODLE_ADD_ANM_set_anm(z);

#undef DOODLE_ADD_ANM_declaration
#undef DOODLE_ADD_ANM_set
#undef DOODLE_ADD_ANM_set_anm
  }
  p_i->dg_modidier.doIt();
}
void dem_bones_comm::set_result() {
  MStatus k_s{};
  MFnIkJoint l_j{};
  MDagPath l_path{};
  for (auto&& i : p_i->joins) {
    k_s = l_j.setObject(i);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_j.getPath(l_path);
    DOODLE_MAYA_CHICK(k_s);
    appendToResult(l_path.fullPathName());
  }
}

dem_bones_comm::~dem_bones_comm() = default;

}  // namespace doodle::maya_plug
