//
// Created by td_main on 2023/4/27.
//

#include "export_file_abc.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/logger/logger.h"

#include <boost/lambda2/lambda2.hpp>

#include <maya_plug/data/cloth_interface.h>

#include "data/cloth_interface.h"
#include "data/export_file_abc.h"
#include "data/maya_tool.h"
#include "data/reference_file.h"
#include "exception/exception.h"
#include <map>
#include <maya/MApiNamespace.h>
#include <maya/MDagPath.h>
#include <maya/MFn.h>
#include <maya/MFnDagNode.h>
#include <maya/MItDependencyGraph.h>
#include <vector>

namespace doodle::maya_plug {

std::vector<MDagPath> export_file_abc::cloth_export_model(const entt::handle_view<reference_file>& in_handle) {
  std::vector<MDagPath> l_export_path{};
  auto& l_ref = in_handle.get<reference_file>();
  MStatus l_status{};
  MFnDagNode l_child_dag{};
  MObject l_export_group{l_ref.export_group_attr()->node(&l_status)};
  maya_chick(l_status);
  for (auto&& [e, l_cloth] : g_reg()->view<cloth_interface>().each()) {
    if (l_cloth->get_namespace() == l_ref.get_namespace()) {
      for (MItDependencyGraph l_it{
               l_export_group, MFn::kMesh, MItDependencyGraph::Direction::kDownstream,
               MItDependencyGraph::Traversal::kDepthFirst, MItDependencyGraph::Level::kNodeLevel, &l_status};
           !l_it.isDone(); l_it.next()) {
        auto l_current_path = get_dag_path(get_transform(l_it.currentItem(&l_status)));
        maya_chick(l_status);
        l_status = l_child_dag.setObject(l_current_path);
        DOODLE_MAYA_CHICK(l_status);
        if (l_child_dag.hasParent(l_export_group)) {
          auto l_path = l_current_path;
          if (auto l_it_j = ranges::find_if(l_export_path, boost::lambda2::_1 == l_path);
              l_it_j == l_export_path.end()) {
            l_export_path.emplace_back(l_current_path);
          }
        }
      }
    }

    return l_export_path;
  }
}

void export_file_abc::export_sim(const entt::handle_view<reference_file, generate_file_path_ptr>& in_handle) {
  auto& l_arg = in_handle.get<generate_file_path_ptr>();
  auto& L_ref = in_handle.get<reference_file>();
  auto l_root = L_ref.export_group_attr();
  if (!l_root) {
    return;
  }
  auto& k_cfg = g_reg()->ctx().get<project_config::base_config>();

  std::map<reference_file_ns::generate_abc_file_path, MSelectionList> export_map{};
  std::vector<MDagPath> export_path{};

  if (k_cfg.use_only_sim_cloth) {
    DOODLE_LOG_INFO("只导出解算物体");
    export_path = cloth_export_model(in_handle);
  } else {
  }
}
}  // namespace doodle::maya_plug