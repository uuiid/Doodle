//
// Created by TD on 2022/3/8.
//

#include "find_duplicate_poly_comm.h"

#include <maya_plug/data/m_namespace.h>

#include "data/qcloth_factory.h"
#include <data/maya_tool.h>
#include <data/reference_file.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgParser.h>
#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MNamespace.h>
#include <maya/MSelectionList.h>

namespace doodle::maya_plug {

MSyntax find_duplicate_poly_comm_syntax() {
  MSyntax l_syntax{};
  l_syntax.addArg(MSyntax::MArgType::kSelectionItem);
  l_syntax.useSelectionAsDefault(true);
  l_syntax.setObjectType(MSyntax::MObjectFormat::kSelectionList);
  return l_syntax;
}
MStatus find_duplicate_poly_comm::doIt(const MArgList& in_list) {
  MStatus l_status{};
  MArgDatabase k_prase{syntax(), in_list, &l_status};
  MString l_name_s{};

  MSelectionList l_list{};
  DOODLE_MAYA_CHICK(k_prase.getObjects(l_list));

  auto l_refs  = reference_file_factory{}.create_ref();
  auto l_cloth = qcloth_factory{}.create_cloth();
  std::map<std::string, entt::handle> l_ref_map{};
  l_ref_map = l_refs |
              ranges::views::transform([](const entt::handle& in_handle) -> std::pair<std::string, entt::handle> {
                return {in_handle.get<reference_file>().get_namespace(), in_handle};
              }) |
              ranges::to<decltype(l_ref_map)>;
  if (l_list.isEmpty()) {
    return l_status;
  }
  MItSelectionList l_it_list{l_list, MFn::kDagNode, &l_status};
  DOODLE_MAYA_CHICK(l_status);
  std::set<std::string> l_set_list;

  for (; !l_it_list.isDone(); l_it_list.next()) {
    MObject l_obj{};
    l_status = l_it_list.getDependNode(l_obj);
    DOODLE_MAYA_CHICK(l_status);
    auto l_name_space = m_namespace::get_namespace_from_name(get_node_name(l_obj));
    DOODLE_MAYA_CHICK(l_status);
    //      if (l_name_space[0] == ':') l_name_space = l_name_space.substr(1);
    l_set_list.emplace(l_name_space);
  }
  DOODLE_LOG_INFO("获取引用 {} 获取选中 {}", l_ref_map, l_set_list);

  for (auto&& i_ns : l_set_list) {
    for (auto l_h : l_cloth) {
      auto l_c = l_h.get<cloth_interface>();
      if (l_c->get_namespace() == i_ns) l_c->rest(l_ref_map[i_ns]);
    }
  }

  return l_status;
}

}  // namespace doodle::maya_plug
