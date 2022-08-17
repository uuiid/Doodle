//
// Created by TD on 2022/3/4.
//

#include "maya_poly_info.h"
#include <maya/MFnMesh.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MDagPath.h>
namespace doodle::maya_plug {

maya_poly_info::maya_poly_info()
    : maya_obj(),
      has_skin(false),
      is_intermediate_obj(false),
      has_cloth(false),
      node_name(),
      node_org_name(){};
maya_poly_info::maya_poly_info(const MObject &in_mesh_object) : maya_poly_info() {
  set_mesh_info(in_mesh_object);
}
bool maya_poly_info::operator==(const maya_poly_info &in_rhs) const {
  return this->node_org_name == in_rhs.node_org_name;
}
bool maya_poly_info::operator!=(const maya_poly_info &in_rhs) const {
  return !(in_rhs == *this);
}
void maya_poly_info::set_mesh_info(const MObject &in_mesh_object) {
  if (in_mesh_object.hasFn(MFn::kMesh)) {
    MFnDagNode l_mesh{};
    MStatus l_status{};
    l_status = l_mesh.setObject(in_mesh_object);
    DOODLE_MAYA_CHICK(l_status);

    this->is_intermediate_obj = l_mesh.isIntermediateObject(&l_status);
    DOODLE_CHICK(l_status);

    l_status = l_mesh.setObject(get_transform(in_mesh_object));
    DOODLE_CHICK(l_status);
    this->node_name = d_str{l_mesh.absoluteName(&l_status)};
    DOODLE_CHICK(l_status);
    maya_obj        = in_mesh_object;

    this->has_skin  = has_skin_cluster(maya_obj);
    this->has_cloth = has_cloth_link(maya_obj);

    node_org_name   = boost::erase_head_copy(
        node_name,
        boost::numeric_cast<std::int32_t>(
            this->node_name.find_last_of(':')));

    std::string l_find_str{};
    if (has_cloth) {
      l_find_str = g_reg()->ctx().at<project_config::base_config>().cloth_proxy_;
      boost::ends_with(node_org_name, l_find_str);
      has_cloth = boost::ends_with(node_org_name, l_find_str);
    } else if (has_skin) {
      l_find_str = g_reg()->ctx().at<project_config::base_config>().simple_module_proxy_;
      boost::ends_with(node_org_name, l_find_str);
      has_skin = boost::ends_with(node_org_name, l_find_str);
    }
    if (!l_find_str.empty()) {
      boost::erase_last(node_org_name, l_find_str);
    }
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
  //    for (MItDependencyGraph l_it_dependency_graph{l_object, MFn::kPluginLocatorNode,
  //                                                  MItDependencyGraph::kDownstream,
  //                                                  MItDependencyGraph::kDepthFirst,
  //                                                  MItDependencyGraph::kNodeLevel, nullptr};
  //         !l_it_dependency_graph.isDone();
  //         l_it_dependency_graph.next()) {
  //      l_path.setObject(l_it_dependency_graph.currentItem(&l_s));
  //      DOODLE_CHICK(l_s);
  //      if (l_path.typeName(&l_s) == "qlClothShape") {
  //        DOODLE_CHICK(l_s);
  //        return true;
  //      }
  //    }
  //  std::uint32_t i{0};
  if (in_object.hasFn(MFn::kMesh)) {
    auto l_obj = get_plug(in_object, "inMesh").source(&l_s).node(&l_s);
    DOODLE_CHICK(l_s);
    l_path.setObject(l_obj);
    DOODLE_CHICK(l_s);
    return l_path.typeName(&l_s) == d_str{"qlClothShape"};
  }

  return false;
}

}  // namespace doodle::maya_plug
