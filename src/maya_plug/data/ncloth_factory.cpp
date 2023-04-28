//
// Created by td_main on 2023/4/27.
//

#include "ncloth_factory.h"

#include <maya_plug/main/maya_plug_fwd.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MNamespace.h>
#include <maya/MObjectArray.h>
#include <maya/MStatus.h>

namespace doodle::maya_plug {

bool ncloth_factory::has_cloth() {
  MStatus l_status{};
  for (MItDependencyNodes i{MFn::kNucleus, &l_status}; !i.isDone(); i.next()) {
    for (MItDependencyNodes i{MFn::kNCloth, &l_status}; !i.isDone(); i.next()) {
      return true;
    }
    return false;
  }
  return false;
}

}  // namespace doodle::maya_plug