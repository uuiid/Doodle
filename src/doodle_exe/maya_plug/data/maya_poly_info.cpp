//
// Created by TD on 2022/3/4.
//

#include "maya_poly_info.h"
#include <maya/MFnMesh.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MDagPath.h>
namespace doodle::maya_plug {

maya_poly_info::maya_poly_info() = default;
maya_poly_info::maya_poly_info(const MObject &in_mesh_object) : maya_poly_info() {
  set_mesh_info(in_mesh_object);
  this->has_skin  = has_skin_cluster(in_mesh_object);
  this->has_cloth = has_cloth_link(in_mesh_object);
}
bool maya_poly_info::operator==(const maya_poly_info &in_rhs) const {
  return std::tie(numVertices,
                  numEdges,
                  numPolygons,
                  numFaceVertices,
                  numUVs,
                  numUVSets) == std::tie(in_rhs.numVertices,
                                         in_rhs.numEdges,
                                         in_rhs.numPolygons,
                                         in_rhs.numFaceVertices,
                                         in_rhs.numUVs,
                                         in_rhs.numUVSets);
}
bool maya_poly_info::operator!=(const maya_poly_info &in_rhs) const {
  return !(in_rhs == *this);
}
void maya_poly_info::set_mesh_info(const MObject &in_mesh_object) {
  if (in_mesh_object.hasFn(MFn::kMesh)) {
    MFnMesh l_mesh{};
    MStatus l_status{};
    l_mesh.setObject(in_mesh_object);
    this->numVertices         = l_mesh.numVertices(&l_status);
    this->numEdges            = l_mesh.numEdges(&l_status);
    this->numPolygons         = l_mesh.numPolygons(&l_status);
    this->numFaceVertices     = l_mesh.numFaceVertices(&l_status);
    //    this->polygonVertexCount = l_mesh.poly;
    this->numUVs              = l_mesh.numUVs(&l_status);
    this->numUVSets           = l_mesh.numUVSets(&l_status);
    this->is_intermediate_obj = l_mesh.isIntermediateObject(&l_status);
    maya_obj                  = in_mesh_object;
  }
}

bool maya_poly_info::has_skin_cluster(const MObject &in_object) {
  MObject l_object{in_object};

//  for (MItDependencyGraph l_it_dependency_graph{l_object, MFn::kSkinClusterFilter,
//                                                MItDependencyGraph::kDownstream,
//                                                MItDependencyGraph::kDepthFirst,
//                                                MItDependencyGraph::kNodeLevel, nullptr};
//       !l_it_dependency_graph.isDone();
//       l_it_dependency_graph.next()) {
//    return true;
//  }
  for (MItDependencyGraph l_it_dependency_graph{l_object, MFn::kSkinClusterFilter,
                                                MItDependencyGraph::kUpstream,
                                                MItDependencyGraph::kDepthFirst,
                                                MItDependencyGraph::kNodeLevel, nullptr};
       !l_it_dependency_graph.isDone();
       l_it_dependency_graph.next()) {
    return true;
  }

  return false;
}
bool maya_poly_info::has_cloth_link(const MObject &in_object) {
  MObject l_object{in_object};
  MStatus l_s{};
  MFnDependencyNode l_path{};
  for (MItDependencyGraph l_it_dependency_graph{l_object, MFn::kPluginLocatorNode,
                                                MItDependencyGraph::kDownstream,
                                                MItDependencyGraph::kDepthFirst,
                                                MItDependencyGraph::kNodeLevel, nullptr};
       !l_it_dependency_graph.isDone();
       l_it_dependency_graph.next()) {
    l_path.setObject(l_it_dependency_graph.currentItem(&l_s));
    DOODLE_CHICK(l_s);
    if (l_path.typeName(&l_s) == "qlClothShape") {
      DOODLE_CHICK(l_s);
      return true;
    }
  }
//  for (MItDependencyGraph l_it_dependency_graph{l_object, MFn::kPluginLocatorNode,
//                                                MItDependencyGraph::kUpstream,
//                                                MItDependencyGraph::kDepthFirst,
//                                                MItDependencyGraph::kNodeLevel, nullptr};
//       !l_it_dependency_graph.isDone();
//       l_it_dependency_graph.next()) {
//    l_path.setObject(l_it_dependency_graph.currentItem(&l_s));
//    DOODLE_CHICK(l_s);
//    if (l_path.typeName(&l_s) == "qlClothShape") {
//      DOODLE_CHICK(l_s);
//      return true;
//    }
//  }

  return false;
}
}  // namespace doodle::maya_plug
