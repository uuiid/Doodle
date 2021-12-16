//
// Created by TD on 2021/12/16.
//

#include "maya_tool.h"
#include <maya/MPlug.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagPath.h>

#include <maya_plug/maya_plug_fwd.h>
namespace doodle::maya_plug {

MPlug get_plug(const MObject& in_node, const std::string& in_name) {
  MStatus k_s{};
  MFnDependencyNode l_node{in_node, &k_s};
  MPlug l_plug{};
  try {
    l_plug = l_node.findPlug(d_str{in_name}, false, &k_s);
    DOODLE_CHICK(k_s);
  } catch (const maya_error& err) {
    if (err.maya_status == MStatus::kInvalidParameter) {
      MFnDagNode l_dag_node{in_node, &k_s};
      DOODLE_CHICK(k_s);
      MDagPath l_path{};
      k_s = l_dag_node.getPath(l_path);
      DOODLE_CHICK(k_s);
      k_s = l_path.extendToShape();
      DOODLE_CHICK(k_s);
      MFnDagNode l_dag_node_shape{l_path, &k_s};
      l_plug = l_dag_node_shape.findPlug(d_str{in_name}, false, &k_s);
      DOODLE_CHICK(k_s)
    } else
      throw;
  }
  return l_plug;
}
}  // namespace doodle::maya_plug
