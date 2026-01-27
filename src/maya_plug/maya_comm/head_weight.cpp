//
// Created by TD on 25-8-28.
//

#include "head_weight.h"

#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <vector>
namespace doodle::maya_plug {

namespace {
// 对称的骨骼
struct sym_bone {
  std::vector<MDagPath> left_bones_;
  std::vector<MDagPath> right_bones_;
};
}  // namespace

class head_weight::impl {
 public:
  impl()  = default;
  ~impl() = default;
  MDagPath M_Head_;
  MDagPath M_HeadBt_;
  MDagPath M_HeadNeck_;
  MDagPath M_HeadTop_;
  MDagPath M_Jaw_;
  MDagPath M_JawA_;
  MDagPath M_JawUpA_;
  MDagPath M_LipMove_;
  MDagPath M_LoLip1_;
  MDagPath M_MidBrow_;
  MDagPath M_Nose_;
  MDagPath M_NoseTip_;
  MDagPath M_UpLip1_;
  MDagPath M_UpLipEndA_;
  sym_bone Brow_;
  sym_bone BrowSec_;
  sym_bone Cheek_;
  sym_bone Ear_;
  sym_bone Jaw_;
  sym_bone JawUp_;
  sym_bone LoLid_;
  sym_bone LoRing_;
  sym_bone MidBrow_;
  sym_bone NasolabialFold_;
  sym_bone NoseWing_;
  sym_bone Nostril_;
  sym_bone UpLid_;
  sym_bone UpperCheek_;
  sym_bone UpRing_;
};

MSyntax head_weight_syntax() {
  MSyntax l_syntax;
  l_syntax.useSelectionAsDefault(true);
  l_syntax.setObjectType(MSyntax::kSelectionList, 1);
  return l_syntax;
}

head_weight::head_weight() : p_impl(std::make_unique<impl>()) {}
head_weight::~head_weight() = default;

MStatus head_weight::doIt(const MArgList& args) {
  MArgDatabase l_arg_db(syntax(), args);
  MSelectionList l_sel_list{};
  CHECK_MSTATUS_AND_RETURN_IT(l_arg_db.getObjects(l_sel_list));
  if (l_sel_list.length() == 0) {
    MGlobal::displayError("请至少选择一个头部模型");
    return MStatus::kFailure;
  }
  CHECK_MSTATUS_AND_RETURN_IT(l_sel_list.getDagPath(0, p_impl->head_mesh_path_));
  if (!p_impl->head_mesh_path_.hasFn(MFn::kMesh)) {
    MGlobal::displayError("请选择一个头部网格模型");
    return MStatus::kFailure;
  }

  return MStatus::kSuccess;
}

}  // namespace doodle::maya_plug