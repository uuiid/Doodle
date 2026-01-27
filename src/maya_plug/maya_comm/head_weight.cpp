//
// Created by TD on 25-8-28.
//

#include "head_weight.h"
namespace doodle::maya_plug {

MSyntax head_weight_syntax() {
  MSyntax l_syntax;
  l_syntax.useSelectionAsDefault(true);
  l_syntax.setObjectType(MSyntax::kSelectionList, 1);
  return l_syntax;
}

MStatus head_weight::doIt(const MArgList& args) { return MStatus::kSuccess; }

}  // namespace doodle::maya_plug