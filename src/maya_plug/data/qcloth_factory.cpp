//
// Created by td_main on 2023/4/27.
//

#include "qcloth_factory.h"

#include <maya_plug/data/qcloth_shape.h>
#include <maya_plug/main/maya_plug_fwd.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MNamespace.h>
#include <maya/MObjectArray.h>
#include <maya/MStatus.h>
namespace doodle::maya_plug {
bool qcloth_factory::has_cloth() {
  MStatus l_status{};
  MObject l_object{};
  for (MItDependencyNodes i{MFn::kPluginLocatorNode, &l_status}; !i.isDone(); i.next()) {
    l_object = i.thisNode(&l_status);
    MFnDependencyNode const k_dep{l_object};
    if (k_dep.typeName(&l_status) == qcloth_shape::qlSolverShape) {
      return true;
    }
  }
  return false;
}
}  // namespace doodle::maya_plug