//
// Created by TD on 2022/7/29.
//

#include "sequence_to_blend_shape.h"
#include <maya/MArgDatabase.h>
#include <maya/MTime.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MAnimControl.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MComputation.h>
#include <maya/MFnMesh.h>
#include <maya/MDagModifier.h>
#include <maya/MFnIkJoint.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnSet.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MEulerRotation.h>
#include <maya/MQuaternion.h>
#include <maya/MDagPath.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MMatrix.h>
namespace doodle {
namespace maya_plug {

namespace sequence_to_blend_shape_ns {
constexpr char startFrame_f[]  = "-sf";
constexpr char startFrame_lf[] = "-startFrame";
constexpr char endFrame_f[]    = "-ef";
constexpr char endFrame_lf[]   = "-endFrame";

constexpr char bindFrame_f[]   = "-bf";
constexpr char bindFrame_lf[]  = "-bindFrame";

constexpr char parent_f[]      = "-p";
constexpr char parent_lf[]     = "-parent";
MSyntax syntax() {
  MSyntax syntax{};
  syntax.addFlag(startFrame_f, startFrame_lf, MSyntax::kTime);
  syntax.addFlag(endFrame_f, endFrame_lf, MSyntax::kTime);
  syntax.addFlag(bindFrame_f, bindFrame_lf, MSyntax::kTime);
  syntax.addFlag(parent_f, parent_lf, MSyntax::kString);
  return syntax;
}

}  // namespace sequence_to_blend_shape_ns

class sequence_to_blend_shape::impl {
 public:
  std::int32_t startFrame_p{0};
  std::int32_t endFrame_p{120};
  std::int32_t bindFrame_p{0};
  MObject parent_tran;
};

sequence_to_blend_shape::sequence_to_blend_shape()
    : p_i(std::make_unique<impl>()) {
}
void sequence_to_blend_shape::get_arg(const MArgList& in_arg) {
  MStatus k_s;
  MArgDatabase k_prase{syntax(), in_arg};

  if (k_prase.isFlagSet(sequence_to_blend_shape_ns::startFrame_f, &k_s)) {
    DOODLE_CHICK(k_s);
    MTime l_value{};
    k_s = k_prase.getFlagArgument(sequence_to_blend_shape_ns::startFrame_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->startFrame_p = l_value.value();
  } else {
    p_i->startFrame_p = MAnimControl::minTime().value();
  }
  if (k_prase.isFlagSet(sequence_to_blend_shape_ns::endFrame_f, &k_s)) {
    DOODLE_CHICK(k_s);
    MTime l_value{};
    k_s = k_prase.getFlagArgument(sequence_to_blend_shape_ns::endFrame_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->endFrame_p = l_value.value();
  } else {
    p_i->endFrame_p = MAnimControl::maxTime().value();
  }
  if (k_prase.isFlagSet(sequence_to_blend_shape_ns::parent_f, &k_s)) {
    DOODLE_CHICK(k_s);
    MString l_value{};
    k_s = k_prase.getFlagArgument(sequence_to_blend_shape_ns::parent_f, 0, l_value);
    DOODLE_CHICK(k_s);
    MSelectionList l_select{};
    k_s = l_select.add(l_value);
    DOODLE_CHICK(k_s);
    MDagPath l_path;

    k_s = l_select.getDagPath(0, l_path);
    DOODLE_CHICK(k_s);

    p_i->parent_tran = l_path.transform(&k_s);
    DOODLE_CHICK(k_s);
  }
  chick_true<doodle_error>(p_i->startFrame_p < p_i->endFrame_p,
                           DOODLE_LOC, "开始帧 {} 大于结束帧 {}",
                           p_i->startFrame_p, p_i->endFrame_p);
}

MStatus sequence_to_blend_shape::doIt(const MArgList& in_arg) {
  return MStatus();
}

sequence_to_blend_shape::~sequence_to_blend_shape() = default;

}  // namespace maya_plug
}  // namespace doodle
