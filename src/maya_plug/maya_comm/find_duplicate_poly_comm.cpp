//
// Created by TD on 2022/3/8.
//

#include "find_duplicate_poly_comm.h"

#include "data/qcloth_factory.h"
#include <data/find_duplicate_poly.h>
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
namespace {
constexpr char name_s[]      = "-ns";
constexpr char name_s_long[] = "-namespace";

}  // namespace
MSyntax find_duplicate_poly_comm_syntax() {
  MSyntax l_syntax{};
  l_syntax.addFlag(name_s, name_s_long, MSyntax::MArgType::kString);
  l_syntax.addArg(MSyntax::MArgType::kSelectionItem);
  l_syntax.useSelectionAsDefault(true);
  l_syntax.setObjectType(MSyntax::MObjectFormat::kSelectionList);
  return l_syntax;
}
MStatus find_duplicate_poly_comm::doIt(const MArgList& in_list) {
  MStatus l_status{};
  MArgDatabase k_prase{syntax(), in_list, &l_status};
  MString l_name_s{};
  if (k_prase.isFlagSet(name_s, &l_status)) {
    DOODLE_MAYA_CHICK(l_status);
    k_prase.getFlagArgument(name_s, 0, l_name_s);
    auto l_list_p = find_duplicate_poly{}(MNamespace::getNamespaceObjects(l_name_s, false, &l_status));
    //  MFnDagNode l_dag_node{};
    MSelectionList l_list{};
    for (auto&& i : l_list_p) {
      DOODLE_MAYA_CHICK(l_list.add(get_transform(i.first)));
      DOODLE_MAYA_CHICK(l_list.add(get_transform(i.second)));
    }
    MGlobal::setActiveSelectionList(l_list);
  } else {
    MSelectionList l_list{};
    DOODLE_MAYA_CHICK(k_prase.getObjects(l_list));
    auto l_refs = reference_file_factory{}.create_ref();
    ranges::for_each(l_refs, [&](entt::handle& in_handle) { in_handle.emplace<find_duplicate_poly>(in_handle); });
    auto l_cloth = qcloth_factory{}.create_cloth();
    std::map<std::string, entt::handle> l_ref_map{};
    l_ref_map = l_refs |
                ranges::views::transform([](const entt::handle& in_handle) -> std::pair<std::string, entt::handle> {
                  return {in_handle.get<reference_file>().get_namespace(), in_handle};
                }) |
                ranges::to<decltype(l_ref_map)>;
    if (l_list.isEmpty()) {
      for (auto l_h : l_cloth) {
        auto l_c = l_h.get<cloth_interface>();
        l_c->rest(l_ref_map[l_c->get_namespace()]);
      }
    } else {
      MItSelectionList l_it_list{l_list, MFn::kDagNode, &l_status};
      DOODLE_MAYA_CHICK(l_status);
      std::set<std::string> l_set_list;
      MFnDependencyNode l_dep_node{};
      for (; !l_it_list.isDone(); l_it_list.next()) {
        MObject l_obj{};
        l_status = l_it_list.getDependNode(l_obj);
        DOODLE_MAYA_CHICK(l_status);

        DOODLE_MAYA_CHICK(l_dep_node.setObject(l_obj));

        auto l_full_name = l_dep_node.absoluteName(&l_status);
        DOODLE_MAYA_CHICK(l_status);
        auto l_name_space = MNamespace::getNamespaceFromName(l_full_name, &l_status);
        DOODLE_MAYA_CHICK(l_status);
        l_set_list.emplace(d_str{l_name_space});
      }

      for (auto&& i_ns : l_set_list) {
        for (auto l_h : l_cloth) {
          auto l_c = l_h.get<cloth_interface>();
          if (l_ref_map.find(i_ns) != l_ref_map.end()) l_c->rest(l_ref_map[i_ns]);
        }
      }
    }
  }

  return l_status;
}

}  // namespace doodle::maya_plug
