//
// Created by td_main on 2023/4/27.
//

#include "qcloth_factory.h"

#include "doodle_core/core/core_help_impl.h"

#include <maya_plug/data/qcloth_shape.h>
#include <maya_plug/main/maya_plug_fwd.h>

#include "cloth_interface.h"
#include "entt/entity/fwd.hpp"
#include "maya_tool.h"
#include <maya/MFnDependencyNode.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MNamespace.h>
#include <maya/MObjectArray.h>
#include <maya/MStatus.h>
#include <memory>
#include <vector>
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
std::vector<cloth_interface> qcloth_factory::create_cloth() const {
  MStatus l_status{};
  MObject l_object{};
  std::vector<cloth_interface> l_ret{};
  for (MItDependencyNodes i{MFn::kPluginLocatorNode, &l_status}; !i.isDone(); i.next()) {
    l_object = i.thisNode(&l_status);
    MFnDependencyNode const k_dep{l_object};
    if (k_dep.typeName(&l_status) == qcloth_shape::qlClothShape) {
      l_ret.emplace_back(std::make_shared<qcloth_shape>(l_object));
      default_logger_raw()->info("获取布料 {}", get_node_name(l_object));
    }
  }
  return l_ret;
}
}  // namespace doodle::maya_plug