//
// Created by TD on 2021/12/13.
//
#include "reference_comm.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/user.h>

// #include <doodle_core/core/app_base.h>

#include <boost/asio/use_future.hpp>

#include "maya_plug/data/qcloth_factory.h"
#include <maya_plug/data/m_namespace.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/maya_tool.h>
#include <maya_plug/data/qcloth_shape.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/data/sim_cover_attr.h>
#include <maya_plug/fmt/fmt_select_list.h>

#include <magic_enum/magic_enum_all.hpp>
#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgParser.h>
#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItSelectionList.h>
#include <maya/MNamespace.h>
#include <maya/MUuid.h>

namespace doodle::maya_plug {

MSyntax set_cloth_cache_path_syntax() {
  MSyntax l_syntax{};
  l_syntax.addArg(MSyntax::MArgType::kSelectionItem);
  l_syntax.useSelectionAsDefault(true);
  l_syntax.setObjectType(MSyntax::MObjectFormat::kSelectionList);
  return l_syntax;
}

MStatus set_cloth_cache_path::doIt(const MArgList& in_list) {
  MStatus l_status{};
  MArgDatabase k_prase{syntax(), in_list, &l_status};
  MString l_name_s{};

  MSelectionList l_list{};
  DOODLE_MAYA_CHICK(k_prase.getObjects(l_list));

  auto l_refs  = reference_file_factory{}.create_ref();
  auto l_cloth = qcloth_factory{}.create_cloth();
  std::map<std::string, reference_file> l_ref_map{};
  l_ref_map = l_refs |
              ranges::views::transform([](const reference_file& in_handle) -> std::pair<std::string, reference_file> {
                return {in_handle.get_namespace(), in_handle};
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
  DOODLE_LOG_INFO("获取引用 {} 获取选中 {}", fmt::join(l_ref_map, "\n"), fmt::join(l_set_list, "\n"));

  for (auto&& i_ns : l_set_list) {
    for (auto l_h : l_cloth) {
      if (!l_ref_map.contains(i_ns)) {
        displayError(conv::to_ms(fmt::format("没有找到 {} 的引用, 使用 doodle_file_info_edit 命令刷新", i_ns)));
        l_status = MStatus::kFailure;
        return l_status;
      }
      if (l_h->get_namespace() == i_ns) l_h->set_cache_folder(l_ref_map[i_ns], false);
    }
  }

  return l_status;
}

}  // namespace doodle::maya_plug
