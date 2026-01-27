//
// Created by TD on 25-8-28.
//

#include "head_weight.h"

#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
namespace doodle::maya_plug {

class head_weight::impl {
 public:
  impl()  = default;
  ~impl() = default;
  MDagPath head_mesh_path_;
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