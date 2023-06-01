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
#include <maya_plug/data/cloth_interface.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/fmt/fmt_dag_path.h>
#include <maya_plug/fmt/fmt_select_list.h>
#include <maya_plug/fmt/fmt_warp.h>

#include <fmt/format.h>
#include <map>
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

std::string export_file_abc::get_abc_exprt_arg() const {
  boost::ignore_unused(this);
  auto& k_cfg = g_reg()->ctx().get<project_config::base_config>();
  std::string l_r{};
  l_r += "-uvWrite ";
  l_r += "-writeFaceSets ";
  l_r += "-wholeFrameGeo ";
  l_r += "-worldSpace ";
  l_r += "-writeUVSets ";
  l_r += "-stripNamespaces ";
  return l_r;
}

void export_file_abc::export_abc(const MSelectionList& in_select, const FSys::path& in_path) {
  auto& k_cfg = g_reg()->ctx().get<project_config::base_config>();

  MStatus k_s{};
  if (in_select.isEmpty()) {
    DOODLE_LOG_INFO("没有找到导出对象");
    return;
  }

  std::vector<std::string> l_export_paths;
  if (k_cfg.use_merge_mesh) {
    MDagPath k_mesh_path{comm_warp::marge_mesh(in_select, m_name)};
    l_export_paths.emplace_back(fmt::format("-root {}", get_node_full_name(k_mesh_path)));
  } else {
    MStringArray l_string_array{};
    k_s = in_select.getSelectionStrings(l_string_array);
    DOODLE_MAYA_CHICK(k_s);
    for (auto i = 0; i < l_string_array.length(); ++i) {
      l_export_paths.emplace_back(fmt::format("-root {}", l_string_array[i]));
    }
  }

  auto l_com = fmt::format(
      R"(
AbcExport -j "-frameRange {} {} {} -dataFormat ogawa {} -file {}";
)",
      begin_time.as(MTime::uiUnit()),  /// \brief 开始时间
      end_time.as(MTime::uiUnit()),    /// \brief 结束时间
      get_abc_exprt_arg(),             /// \brief 导出参数
      fmt::join(l_export_paths, " "),  /// \brief 导出物体的根路径
      in_path.generic_string()
  );
  DOODLE_LOG_INFO("生成导出命令 {}", l_com);

  /// \brief 导出abc命令
  k_s = MGlobal::executeCommand(d_str{l_com});  /// \brief 导出文件路径，包含文件名和文件路径
  DOODLE_MAYA_CHICK(k_s);
}

void export_file_abc::export_sim(const entt::handle_view<reference_file, generate_file_path_ptr>& in_handle) {
  auto& l_arg = in_handle.get<generate_file_path_ptr>();
  auto& L_ref = in_handle.get<reference_file>();
  auto l_root = L_ref.export_group_attr();
  if (!l_root) {
    return;
  }
  begin_time  = l_arg->begin_end_time.first;
  end_time    = l_arg->begin_end_time.second;
  m_name      = L_ref.get_namespace();

  auto& k_cfg = g_reg()->ctx().get<project_config::base_config>();

  std::map<reference_file_ns::generate_abc_file_path, MSelectionList> export_map{};
  std::vector<MDagPath> export_path{};

  if (k_cfg.use_divide_group_export) {
    ranges::for_each(export_path, [](MDagPath& in) { in.pop(); });

    auto l_suffix = g_reg()->ctx().get<project_config::base_config>().maya_out_put_abc_suffix;
    export_path   = find_out_group_child_suffix_node(*l_root, l_suffix);
    export_path |=
        ranges::actions::unique([](const MDagPath& in_r, const MDagPath& in_l) -> bool { return in_r == in_l; });
    DOODLE_LOG_INFO("按组划分导出收集完成 {}", fmt::join(export_path, " "));
    for (auto&& i : export_path) {
      reference_file_ns::generate_abc_file_path l_name{*g_reg()};
      auto l_node_name = get_node_name_strip_name_space(i);
      if (auto l_it = l_node_name.find(l_suffix); l_it != std::string::npos) {
        l_node_name.erase(l_it, l_suffix.length());
      }

      l_name.add_external_string = l_node_name;
      l_name.begin_end_time      = l_arg->begin_end_time;
      export_map[l_name].add(i, MObject::kNullObj, true);
    }
  } else {
    if (k_cfg.use_only_sim_cloth) {
      DOODLE_LOG_INFO("只导出解算物体");
      export_path = L_ref.get_alll_cloth_obj();
    } else {
      export_path = child_export_model(*l_root);
    }
    DOODLE_LOG_INFO("导出收集完成 {}", fmt::join(export_path, " "));
    MSelectionList l_list{};
    for (auto&& i : export_path) {
      maya_chick(l_list.add(i));
    }
    reference_file_ns::generate_abc_file_path l_name{*g_reg()};
    l_name.begin_end_time = l_arg->begin_end_time;
    export_map[l_name]    = l_list;
  }
  DOODLE_LOG_INFO("导出划分完成 {}", fmt::join(export_map, " "));

  for (auto&& [name, s_l] : export_map) {
    auto l_path = name(L_ref);
    if (!s_l.isEmpty())
      export_abc(s_l, l_path);
    else
      DOODLE_LOG_INFO("没有找到导出对象 {} ", l_path);
  }
}
}  // namespace doodle::maya_plug