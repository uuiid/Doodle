//
// Created by TD on 25-1-2.
//

#include "inspect_file.h"

#include "doodle_core/core/app_base.h"

#include <doodle_lib/exe_warp/maya_exe.h>

#include <maya_plug/data/m_namespace.h>
#include <maya_plug/data/maya_clear_scenes.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/fmt/fmt_dag_path.h>
#include <maya_plug/fmt/fmt_select_list.h>
#include <maya_plug/fmt/fmt_warp.h>

#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyGraph.h>
namespace doodle::maya_plug {
bool inspect_file::post(const nlohmann::json& in_argh) {
  auto l_arg = in_argh.get<maya_exe_ns::inspect_file_arg>();
  if (l_arg.file_path.empty()) return false;
  maya_file_io::set_workspace(l_arg.file_path);
  maya_file_io::open_file(l_arg.file_path, MFileIO::kLoadDefault);
  MStatus l_s{};
  maya_enum::maya_error_t l_e = maya_enum::maya_error_t::success;
  if (l_arg.surface_5_) {
    default_logger_raw()->info("开始检查五边面");
    MSelectionList l_select{};
    if (maya_clear_scenes::multilateral_surface(l_select)) {
      default_logger_raw()->error("存在五边面 {}", l_select);
      l_e = maya_enum::maya_error_t::check_error;
    }
  }
  if (l_arg.rename_check_) {
    default_logger_raw()->info("开始检查重名");
    MSelectionList l_select{};
    // for (MItDag l_iter{MItDag::kDepthFirst, MFn::kTransform, &l_s}; !l_iter.isDone(); l_iter.next())
    //   l_select.add(l_iter.currentItem());

    if (maya_clear_scenes::duplicate_name(l_select)) {
      default_logger_raw()->error("存在重名 {}", l_select);
      l_e = maya_enum::maya_error_t::check_error;
    }
  }
  if (l_arg.name_length_check_) {
    default_logger_raw()->info("开始名称长度");
    MDagPath l_dag_path{};
    MFnDagNode l_dag_node{};

    for (MItDag l_iter{MItDag::kDepthFirst, MFn::kTransform, &l_s}; !l_iter.isDone(); l_iter.next()) {
      maya_chick(l_iter.getPath(l_dag_path));
      maya_chick(l_dag_node.setObject(l_dag_path));
      auto l_name = l_dag_node.name(&l_s);
      maya_chick(l_s);
      if (l_name.length() > 45) {
        default_logger_raw()->error("存在超长名称 {}", l_dag_path);
        l_e = maya_enum::maya_error_t::check_error;
      }
    }
  }

  if (l_arg.history_check_) {
    default_logger_raw()->info("开始检查历史记录");
    for (MItDag l_iter{MItDag::kDepthFirst, MFn::kTransform, &l_s}; !l_iter.isDone(); l_iter.next()) {
      MObject l_root = l_iter.currentItem();
      std::int32_t l_begin{};
      for (MItDependencyGraph l_dep_it{l_root, MFn::kInvalid, MItDependencyGraph::kUpstream}; !l_dep_it.isDone();
           l_dep_it.next()) {
        ++l_begin;
        if (l_begin > 2) {
          default_logger_raw()->error("存在历史记录 {}", get_node_full_name(l_root));
          l_e = maya_enum::maya_error_t::check_error;
          break;
        }
      }
      l_begin = {0};
    }
  }

  if (l_arg.special_copy_check_) {
    default_logger_raw()->info("开始检查特殊拷贝");
    MDagPath l_dag_path{};
    // MFnDagNode l_dag_node{};
    for (MItDag l_iter{MItDag::kDepthFirst, MFn::kMesh, &l_s}; !l_iter.isDone(); l_iter.next()) {
      maya_chick(l_iter.getPath(l_dag_path));
      // maya_chick(l_dag_node.setObject(l_dag_path));
      // default_logger_raw()->info("检查特殊拷贝 {}", l_dag_path);
      if (l_dag_path.isInstanced(&l_s)) {
        maya_chick(l_s);
        default_logger_raw()->error("存在特殊拷贝 {}", l_dag_path);
        l_e = maya_enum::maya_error_t::check_error;
      }
    }
  }

  if (l_arg.uv_check_) {
    default_logger_raw()->info("开始检查UV");
    MFnMesh l_mesh{};
    for (MItDag l_iter{MItDag::kDepthFirst, MFn::kMesh, &l_s}; !l_iter.isDone(); l_iter.next()) {
      maya_chick(l_mesh.setObject(l_iter.currentItem()));
      if (l_mesh.numUVSets() == 0) {
        default_logger_raw()->error("存在UV缺失 {}", get_node_full_name(l_mesh.object()));
        l_e = maya_enum::maya_error_t::check_error;
      }
      std::vector<std::int32_t> l_polygon{};
      for (auto l_i = 0; l_i < l_mesh.numPolygons(); ++l_i) {
        MIntArray l_vertices{};
        maya_chick(l_mesh.getPolygonVertices(l_i, l_vertices));
        MFloatPoint l_point_org, l_point_x, l_point_y{};
        if (l_vertices.length() < 3) {
          default_logger_raw()->error("存在缺失的面 {}", get_node_full_name(l_mesh.object()));
          l_e = maya_enum::maya_error_t::check_error;
          continue;
        }
        maya_chick(l_mesh.getPolygonUV(l_i, 0, l_point_x.x, l_point_x.y));
        maya_chick(l_mesh.getPolygonUV(l_i, 1, l_point_org.x, l_point_org.y));
        maya_chick(l_mesh.getPolygonUV(l_i, 2, l_point_y.x, l_point_y.y));
        MVector l_x{l_point_x - l_point_org}, l_y{l_point_y - l_point_org};
        if ((l_x ^ l_y) * MVector::zAxis < 0) {
          l_polygon.emplace_back(l_i);
        }
      }
      if (l_polygon.size() > 0) {
        default_logger_raw()->error(
            "存在反面 {}:\n {}", get_node_full_name(l_mesh.object()),
            fmt::join(l_polygon | ranges::views::chunk(10), "\n")
        );
      }
    }
  }

  if (l_arg.kframe_check_) {
    default_logger_raw()->info("开始检查模型关键帧");

    for (MItDag l_iter{MItDag::kDepthFirst, MFn::kTransform, &l_s}; !l_iter.isDone(); l_iter.next()) {
      MObject l_root = l_iter.currentItem();
      for (MItDependencyGraph l_dep_it{l_root, MFn::kAnimCurve, MItDependencyGraph::kUpstream}; !l_dep_it.isDone();
           l_dep_it.next()) {
        default_logger_raw()->error("存在动画K帧 {}", get_node_full_name(l_root));
        l_e = maya_enum::maya_error_t::check_error;
        break;
      }
    }
  }

  if (l_arg.space_name_check_) {
    default_logger_raw()->info("开始检查空间名称");

    for (MItDag l_iter{MItDag::kDepthFirst, MFn::kTransform, &l_s}; !l_iter.isDone(); l_iter.next()) {
      auto l_full_name = get_node_full_name(l_iter.currentItem());
      auto l_sp        = m_namespace::get_namespace_from_name(l_full_name);
      if (!l_sp.empty()) {
        default_logger_raw()->error("存在空间名称 {}", l_full_name);
        l_e = maya_enum::maya_error_t::check_error;
      }
    }
  }

  if (l_arg.only_default_camera_check_) {
    default_logger_raw()->info("开始检查默认相机");
    std::array<std::int8_t, 5> l_checks{};
    for (MItDag l_iter{MItDag::kDepthFirst, MFn::kCamera, &l_s}; !l_iter.isDone(); l_iter.next()) {
      auto l_name = get_node_full_name(l_iter.currentItem());
      // side front top persp
      l_checks[0] += l_name.starts_with("|side");
      l_checks[1] += l_name.starts_with("|front");
      l_checks[2] += static_cast<int>(l_name.starts_with("|top"));
      l_checks[3] += static_cast<int>(l_name.starts_with("|persp"));
      ++l_checks[4];
      if (l_checks[0] > 1 || l_checks[1] > 1 || l_checks[2] > 1 || l_checks[3] > 1 || l_checks[4] > 4) {
        default_logger_raw()->error("存在多个默认相机 {}", l_name);
        l_e = maya_enum::maya_error_t::check_error;
      }
    }
  }
  if (l_arg.too_many_point_check_) {
    default_logger_raw()->info("检查多余点数功能暂时不可用");
  }
  app_base::Get().stop_app();
  if (l_e == maya_enum::maya_error_t::check_error)
    throw_error(l_e);
  return false;
}

}  // namespace doodle::maya_plug






























