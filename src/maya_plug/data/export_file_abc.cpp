//
// Created by td_main on 2023/4/27.
//

#include "export_file_abc.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/logger/logger.h"

#include <boost/core/ignore_unused.hpp>
#include <boost/lambda2/lambda2.hpp>

#include "maya_plug/data/cloth_interface.h"
#include "maya_plug/data/export_file_abc.h"
#include "maya_plug/data/m_namespace.h"
#include "maya_plug/data/maya_conv_str.h"
#include "maya_plug/data/maya_tool.h"
#include "maya_plug/data/reference_file.h"
#include "maya_plug/exception/exception.h"
#include <maya_plug/abc/alembic_archive_out.h>
#include <maya_plug/data/cloth_interface.h>
#include <maya_plug/data/dagpath_cmp.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/fmt/fmt_dag_path.h>
#include <maya_plug/fmt/fmt_select_list.h>
#include <maya_plug/fmt/fmt_warp.h>

#include "range/v3/range/conversion.hpp"
#include <cmath>
#include <fmt/format.h>
#include <map>
#include <maya/MAnimControl.h>
#include <maya/MApiNamespace.h>
#include <maya/MDagPath.h>
#include <maya/MFn.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MNamespace.h>
#include <maya/MObject.h>
#include <range/v3/algorithm/for_each.hpp>
#include <set>
#include <vector>

namespace doodle::maya_plug {

std::vector<MDagPath> export_file_abc::child_export_model(const MDagPath& in_root) {
  std::vector<MDagPath> l_export_path{};
  MStatus l_status{};
  MItDag k_it{};
  maya_chick(k_it.reset(in_root, MItDag::kDepthFirst, MFn::Type::kMesh));
  MFnDagNode l_fn_dag_node{};
  MDagPath l_path{};
  for (; !k_it.isDone(&l_status); k_it.next()) {
    maya_chick(l_status);
    l_status = k_it.getPath(l_path);
    maya_chick(l_status);

    l_status = l_fn_dag_node.setObject(l_path);
    maya_chick(l_status);
    /// \brief 检查一下是否是中间对象
    if (!l_fn_dag_node.isIntermediateObject(&l_status)) {
      maya_chick(l_status);
      l_export_path.emplace_back(l_path);
    }
  }
  return l_export_path;
}
std::vector<MDagPath> export_file_abc::find_out_group_child_suffix_node(
    const MDagPath& in_root, const std::string& in_suffix
) {
  MItDag l_it{};
  MStatus l_status{};
  l_status = l_it.reset(in_root, MItDag::kBreadthFirst, MFn::Type::kTransform);
  maya_chick(l_status);

  std::vector<MDagPath> l_r{};
  for (; !l_it.isDone(); l_it.next()) {
    MDagPath l_path{};
    l_status = l_it.getPath(l_path);
    DOODLE_MAYA_CHICK(l_status);
    DOODLE_LOG_DEBUG("迭代路径 {}", l_path);
    auto l_node = get_node_name_strip_name_space(l_path);
    if (l_node.length() > in_suffix.length() && std::equal(in_suffix.rbegin(), in_suffix.rend(), l_node.rbegin())) {
      l_r.emplace_back(l_path);
    }
  }

  return l_r;
}

void export_file_abc::export_abc(const MSelectionList& in_select, const FSys::path& in_path) {
  DOODLE_LOG_INFO("导出物体 {} 路径 {}", in_select, in_path);
  MAnimControl::setCurrentTime(begin_time);
  MStatus l_status{};
  MItDag k_it{};

  std::set<MDagPath, details::cmp_dag> l_export_set{};

  for (auto i = 0; i < in_select.length(); ++i) {
    MDagPath k_path{};
    in_select.getDagPath(i, k_path);

    /// 收集所有的子网格
    maya_chick(k_it.reset(k_path, MItDag::kDepthFirst, MFn::Type::kMesh));
    MFnDagNode l_fn_dag_node{};
    for (; !k_it.isDone(&l_status); k_it.next()) {
      maya_chick(l_status);
      maya_chick(k_it.getPath(k_path));
      maya_chick(l_fn_dag_node.setObject(k_path));

      /// \brief 检查一下是否是中间对象
      if (!l_fn_dag_node.isIntermediateObject(&l_status)) {
        maya_chick(l_status);
        l_export_set.emplace(k_path);
      }
    }
    //    l_dag_path.emplace_back(k_path);
  }
  std::vector<MDagPath> l_dag_path = l_export_set | ranges::to_vector;
  DOODLE_LOG_INFO("收集完成导出物体 {}", l_dag_path);

  alembic::archive_out l_out{in_path, l_dag_path, begin_time, end_time};

  for (auto i = begin_time; i <= end_time; ++i) {
    MAnimControl::setCurrentTime(i);
    l_out.write();
  }
}

std::vector<FSys::path> export_file_abc::export_sim(
    const entt::handle_view<reference_file, generate_file_path_ptr>& in_handle
) {
  auto& l_arg = in_handle.get<generate_file_path_ptr>();
  auto& L_ref = in_handle.get<reference_file>();
  auto l_root = L_ref.export_group_attr();
  if (!l_root) {
    return {};
  }
  begin_time = l_arg->begin_end_time.first;
  end_time   = l_arg->begin_end_time.second;
  m_name     = L_ref.get_namespace();

  std::map<reference_file_ns::generate_abc_file_path, MSelectionList> export_map{};
  std::vector<MDagPath> export_path{};

  DOODLE_LOG_INFO("只导出解算物体");
  export_path = L_ref.get_alll_cloth_obj();
  DOODLE_LOG_INFO("导出收集完成 {}", fmt::join(export_path, " "));
  MSelectionList l_list{};
  for (auto&& i : export_path) {
    maya_chick(l_list.add(i));
  }
  reference_file_ns::generate_abc_file_path l_name{*g_reg()};
  l_name.begin_end_time = l_arg->begin_end_time;
  export_map[l_name]    = l_list;
  DOODLE_LOG_INFO("导出划分完成 {}", fmt::join(export_map, " "));
  l_current_export_list.clear();
  std::vector<FSys::path> l_path_ret{};
  for (auto&& [name, s_l] : export_map) {
    auto l_path = name(L_ref);
    l_current_export_list.merge(s_l);
    if (!s_l.isEmpty()) {
      export_abc(s_l, l_path);
      l_path_ret.emplace_back(l_path);
    } else
      DOODLE_LOG_INFO("没有找到导出对象 {} ", l_path);
  }
  return l_path_ret;
}
}  // namespace doodle::maya_plug