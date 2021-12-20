//
// Created by TD on 2021/12/16.
//

#include "maya_tool.h"
#include <maya/MPlug.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagPath.h>
#include <maya/MItDependencyGraph.h>

#include <maya_plug/maya_plug_fwd.h>
namespace doodle::maya_plug {

MPlug get_plug(const MObject& in_node, const std::string& in_name) {
  MStatus k_s{};
  MFnDependencyNode l_node{in_node, &k_s};
  MPlug l_plug{};
  if (in_node.hasFn(MFn::kDagNode)) {
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
  } else if (in_node.hasFn(MFn::kDependencyNode)) {
    l_plug = l_node.findPlug(d_str{in_name}, false, &k_s);
    DOODLE_CHICK(k_s);
  } else {
    throw doodle_error{"无法附加到功能集"};
  }
  return l_plug;
}
MObject get_shading_engine(const MObject& in_node) {
  MStatus k_s{};
  MObject k_obj = in_node;
  if (in_node.hasFn(MFn::kDagNode)) {
    MFnDagNode l_dag_node{in_node, &k_s};
    DOODLE_CHICK(k_s);
    MDagPath l_path{};
    k_s = l_dag_node.getPath(l_path);
    DOODLE_CHICK(k_s);
    k_s = l_path.extendToShape();
    DOODLE_CHICK(k_s);
    k_obj = l_path.node(&k_s);
    DOODLE_CHICK(k_s);
  }

  for (MItDependencyGraph i{k_obj,
                            MFn::Type::kShadingEngine,
                            MItDependencyGraph::Direction::kDownstream,
                            MItDependencyGraph::Traversal::kDepthFirst,
                            MItDependencyGraph::Level::kNodeLevel,
                            &k_s};
       !i.isDone();
       i.next()) {
    DOODLE_CHICK(k_s);
    auto obj = i.currentItem(&k_s);
    //    DOODLE_CHICK(k_s);
    //    MFnDependencyNode k_node{};
    //    k_node.setObject(obj);
    //    DOODLE_LOG_INFO(fmt::format("找到节点 {}", d_str{k_node.name()}.str()));
    return obj;
  }
//  for (MItDependencyGraph i{k_obj,
//                            MFn::Type::kInvalid,
//                            MItDependencyGraph::Direction::kUpstream,
//                            MItDependencyGraph::Traversal::kDepthFirst,
//                            MItDependencyGraph::Level::kNodeLevel,
//                            &k_s};
//       !i.isDone();
//       i.next()) {
//    DOODLE_CHICK(k_s);
//    auto obj = i.currentItem(&k_s);
//    DOODLE_CHICK(k_s);
//    MFnDependencyNode k_node{};
//    k_node.setObject(obj);
//    DOODLE_LOG_INFO(fmt::format("找到节点 {}", d_str{k_node.name()}.str()));
//    //    return obj;
//  }
  throw doodle_error{"没有找到节点"};
}
MObject get_first_mesh(const MObject& in_node) {
  MStatus k_s{};
  MObject k_obj = in_node;
  if (in_node.hasFn(MFn::kDagNode)) {
    MFnDagNode l_dag_node{in_node, &k_s};
    DOODLE_CHICK(k_s);
    MDagPath l_path{};
    k_s = l_dag_node.getPath(l_path);
    DOODLE_CHICK(k_s);
    k_s = l_path.extendToShape();
    DOODLE_CHICK(k_s);
    k_obj = l_path.node(&k_s);
    DOODLE_CHICK(k_s);
    return k_obj;
  }
  for (MItDependencyGraph i{k_obj,
                            MFn::Type::kMesh,
                            MItDependencyGraph::Direction::kDownstream,
                            MItDependencyGraph::Traversal::kDepthFirst,
                            MItDependencyGraph::Level::kNodeLevel,
                            &k_s};
       !i.isDone();
       i.next()) {
    DOODLE_CHICK(k_s);
    auto obj = i.currentItem(&k_s);
    return obj;
  }
  return MObject();
}
}  // namespace doodle::maya_plug
