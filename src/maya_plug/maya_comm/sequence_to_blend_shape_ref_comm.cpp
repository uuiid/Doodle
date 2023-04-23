//
// Created by TD on 2022/8/15.
//

#include "sequence_to_blend_shape_ref_comm.h"

#include <doodle_core/lib_warp/std_fmt_system_error.h>

#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/qcloth_shape.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/data/sequence_to_blend_shape.h>

#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MBoundingBox.h>
#include <maya/MDGContextGuard.h>
#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MDataHandle.h>
#include <maya/MEulerRotation.h>
#include <maya/MFnIkJoint.h>
#include <maya/MFnMesh.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItMeshVertex.h>
#include <maya/MMatrix.h>
#include <maya/MNamespace.h>
#include <maya/MQuaternion.h>
#include <maya/MSelectionList.h>

namespace doodle::maya_plug {

namespace sequence_to_blend_shape_ref_comm_ns {
constexpr char startFrame_f[]  = "-sf";
constexpr char startFrame_lf[] = "-startFrame";
constexpr char endFrame_f[]    = "-ef";
constexpr char endFrame_lf[]   = "-endFrame";

MSyntax syntax() {
  MSyntax syntax{};
  syntax.addFlag(startFrame_f, startFrame_lf, MSyntax::kTime);
  syntax.addFlag(endFrame_f, endFrame_lf, MSyntax::kTime);
  /// \brief 选中的物体
  syntax.setObjectType(MSyntax::MObjectFormat::kSelectionList);
  syntax.useSelectionAsDefault(true);
  syntax.enableEdit(false);
  syntax.enableQuery(false);

  return syntax;
}

}  // namespace sequence_to_blend_shape_ref_comm_ns

class sequence_to_blend_shape_ref_comm::impl {
 public:
  std::int32_t startFrame_p{0};
  std::int32_t endFrame_p{120};

  bool duplicate_bind{};

  MSelectionList select_list;

  MDagModifier dg_modidier;

  std::vector<sequence_to_blend_shape> blend_list{};
};
void sequence_to_blend_shape_ref_comm::get_arg(const MArgList& in_arg) {
  MStatus k_s;
  MArgDatabase k_prase{syntax(), in_arg};

  if (k_prase.isFlagSet(sequence_to_blend_shape_ref_comm_ns::startFrame_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    MTime l_value{};
    k_s = k_prase.getFlagArgument(sequence_to_blend_shape_ref_comm_ns::startFrame_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->startFrame_p = boost::numeric_cast<std::int32_t>(l_value.value());
  } else {
    p_i->startFrame_p = g_reg()->ctx().get<project_config::base_config>().export_anim_time;
  }
  if (k_prase.isFlagSet(sequence_to_blend_shape_ref_comm_ns::endFrame_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    MTime l_value{};
    k_s = k_prase.getFlagArgument(sequence_to_blend_shape_ref_comm_ns::endFrame_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->endFrame_p = boost::numeric_cast<std::int32_t>(l_value.value());
  } else {
    p_i->endFrame_p = boost::numeric_cast<std::int32_t>(MAnimControl::maxTime().value());
  }

  p_i->startFrame_p < p_i->endFrame_p
      ? void()
      : throw_exception(doodle_error{"开始帧 {} 大于结束帧 {}"s, p_i->startFrame_p < p_i->endFrame_p});
  /// \brief 获取选择物体
  k_s = k_prase.getObjects(p_i->select_list);
  DOODLE_MAYA_CHICK(k_s);

  /// \brief 生成绑定物体path
  DOODLE_LOG_INFO("开始生成新的布料组件");

  if (p_i->select_list.length() > 0) {
    DOODLE_LOG_INFO("使用交互式创建混合变形");
    for (auto&& [e, ref] : g_reg()->view<reference_file>().each()) {
      DOODLE_LOG_INFO("测试引用文件 {}", ref.path);
      if (ref.has_node(p_i->select_list)) {
        DOODLE_LOG_INFO("测试选择命中引用文件 {}", ref.path);
        maya_file_io::import_reference_file(ref, false);
        auto l_p = ref.export_group_attr();
        for (auto&& ql_export : ref.qcloth_export_model()) {
          sequence_to_blend_shape l_blend_shape{};
          l_blend_shape.select_attr(ql_export);
          if (l_p) l_blend_shape.parent_attr(*l_p);
          p_i->blend_list.emplace_back(std::move(l_blend_shape));
        }
      }
    }
  } else {
    DOODLE_LOG_INFO("使用批量自动运行创建混合变形");
    for (auto&& [e, ref] : g_reg()->view<reference_file>().each()) {
      DOODLE_LOG_INFO("开始转换引用文件 {}", ref.path);
      maya_file_io::import_reference_file(ref, false);
      auto l_p = ref.export_group_attr();
      for (auto&& ql_export : ref.qcloth_export_model()) {
        sequence_to_blend_shape l_blend_shape{};
        l_blend_shape.select_attr(ql_export);
        if (l_p) l_blend_shape.parent_attr(*l_p);
        p_i->blend_list.emplace_back(std::move(l_blend_shape));
      }
    }
  }
}
void sequence_to_blend_shape_ref_comm::create_mesh() {
  MStatus l_s{};

  {  /// \brief 设置时间
    l_s = MGlobal::viewFrame(p_i->startFrame_p);
    DOODLE_MAYA_CHICK(l_s);

    for (auto&& ctx : p_i->blend_list) {
      DOODLE_LOG_INFO("开始创建绑定网格 {}", get_node_name(ctx.select_attr()));
      ctx.create_bind_mesh();
    }
  }

  for (auto i = p_i->startFrame_p; i <= p_i->endFrame_p; ++i) {
    l_s = MAnimControl::setCurrentTime(MTime{boost::numeric_cast<std::double_t>(i), MTime::uiUnit()});
    DOODLE_MAYA_CHICK(l_s);
    for (auto&& ctx : p_i->blend_list) {
      ctx.create_blend_shape_mesh();
    }
  }
}
void sequence_to_blend_shape_ref_comm::create_anim() {
  for (auto&& ctx : p_i->blend_list) {
    DOODLE_LOG_INFO("开始创建绑定网格 {} 的动画", get_node_name(ctx.select_attr()));

    ctx.create_blend_shape_anim(p_i->startFrame_p, p_i->endFrame_p, p_i->dg_modidier);
  }
}
void sequence_to_blend_shape_ref_comm::run_blend_shape_comm() {
  for (auto&& ctx : p_i->blend_list) {
    DOODLE_LOG_INFO("开始创建绑定网格 {} 的混合变形", get_node_name(ctx.select_attr()));
    ctx.create_blend_shape();
  }
}
void sequence_to_blend_shape_ref_comm::add_to_parent() {
  for (auto&& ctx : p_i->blend_list) {
    try {
      ctx.attach_parent();
    } catch (const std::runtime_error& error) {
      DOODLE_LOG_WARN("由于错误 {} 取消附加", error);
    }
  }
}
sequence_to_blend_shape_ref_comm::sequence_to_blend_shape_ref_comm() : p_i(std::make_unique<impl>()) {}
sequence_to_blend_shape_ref_comm::~sequence_to_blend_shape_ref_comm() = default;
MStatus sequence_to_blend_shape_ref_comm::doIt(const MArgList& in_arg) {
  get_arg(in_arg);
  return redoIt();
}
MStatus sequence_to_blend_shape_ref_comm::undoIt() {
  for (auto&& ctx : p_i->blend_list) {
    ctx.delete_bind_mesh();
  }
  return MStatus::kSuccess;
}
MStatus sequence_to_blend_shape_ref_comm::redoIt() {
  try {
    this->create_mesh();
    this->run_blend_shape_comm();
    this->create_anim();
    this->add_to_parent();
    this->delete_node();
  } catch (const std::runtime_error& err) {
    DOODLE_LOG_WARN(boost::diagnostic_information(err));
    return MStatus::kFailure;
  }
  return MStatus::kSuccess;
}
bool sequence_to_blend_shape_ref_comm::isUndoable() const { return true; }
void sequence_to_blend_shape_ref_comm::delete_node() {
  for (auto&& ctx : p_i->blend_list) {
    DOODLE_LOG_INFO("开始删除 {} 的原始模型", get_node_name(ctx.select_attr()));
    ctx.delete_select_node();
  }
}
}  // namespace doodle::maya_plug
