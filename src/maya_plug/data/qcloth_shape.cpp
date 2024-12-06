//
// Created by TD on 2021/12/6.
//

#include "qcloth_shape.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/logger/logger.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/functional/factory.hpp>
#include <boost/functional/value_factory.hpp>

#include <maya_plug/data/m_namespace.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/maya_tool.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/fmt/fmt_dag_path.h>
#include <maya_plug/main/maya_plug_fwd.h>

#include "data/maya_conv_str.h"
#include "data/qcloth_shape.h"
#include "data/sim_cover_attr.h"
#include "entt/entity/fwd.hpp"
#include "exception/exception.h"
#include <fmt/core.h>
#include <magic_enum.hpp>
#include <maya/MAnimControl.h>
#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItSelectionList.h>
#include <maya/MNamespace.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <string>

namespace doodle::maya_plug {

namespace qcloth_shape_n {
maya_obj::maya_obj() = default;
maya_obj::maya_obj(const MObject& in_object) {
  obj = in_object;
  MStatus k_s;
  MFnDependencyNode k_node{in_object, &k_s};
  DOODLE_MAYA_CHICK(k_s);
  p_name = d_str{k_node.name(&k_s)};
  DOODLE_MAYA_CHICK(k_s);
}
}  // namespace qcloth_shape_n

qcloth_shape::qcloth_shape() = default;
qcloth_shape::qcloth_shape(const MObject& in_object) : obj(in_object) {};

void qcloth_shape::set_cache_folder(const entt::handle& in_handle, const FSys::path& in_path, bool need_clear) const {
  std::string k_namespace = in_handle.get<reference_file>().get_namespace();
  std::string k_node_name = m_namespace::strip_namespace_from_name(get_node_full_name(obj));

  auto k_cache            = get_plug(obj, "cacheFolder");
  auto k_file_name        = maya_file_io::get_current_path();
  FSys::path l_string = fmt::format("cache/{}/{}/{}", k_file_name.stem().generic_string(), k_namespace, k_node_name);
  l_string /= in_path;
  DOODLE_LOG_INFO("设置缓存路径 {}", l_string);
  auto k_path = maya_file_io::work_path(l_string);
  if (need_clear && FSys::exists(k_path)) {
    DOODLE_LOG_INFO("发现缓存目录, 主动删除 {}", k_path);
    FSys::remove_all(k_path);
  }
  FSys::create_directories(k_path);
  set_attribute(obj, "cacheFolder", l_string.generic_string());
  set_attribute(obj, "cacheName", k_node_name);
}

bool qcloth_shape::chick_low_skin(const entt::handle& in_handle) {
  in_handle.any_of<qcloth_shape_n::maya_obj>() ? void() : throw_exception(doodle_error{"缺失组件"s});
  MStatus l_s{};
  auto l_shape = maya_plug::get_shape(in_handle.get<qcloth_shape_n::maya_obj>().obj);
  /// 寻找高模的皮肤簇
  for (MItDependencyGraph i{l_shape, MFn::kSkinClusterFilter, MItDependencyGraph::Direction::kUpstream}; !i.isDone();
       i.next()) {
    DOODLE_MAYA_CHICK(l_s);
    return true;
  }

  return false;
}
MObject qcloth_shape::get_ql_solver(const MSelectionList& in_selection_list) {
  MStatus l_status{};
  MObject l_object{};

  for (MItSelectionList i{in_selection_list, MFn::kPluginLocatorNode, &l_status}; !i.isDone(); i.next()) {
    DOODLE_MAYA_CHICK(l_status);
    l_status = i.getDependNode(l_object);
    DOODLE_MAYA_CHICK(l_status);
    MFnDependencyNode k_dep{l_object};
    if (k_dep.typeName(&l_status) == qlSolverShape) {
      default_logger_raw()->info("找到解算核心 {}", get_node_full_name(l_object));
      break;
    }
  }
  default_logger_raw()->info("未找到解算核心");

  return l_object;
}
MObject qcloth_shape::get_ql_solver() {
  MStatus l_status{};
  MObject l_object{};
  for (MItDependencyNodes i{MFn::kPluginLocatorNode, &l_status}; !i.isDone(); i.next()) {
    l_object = i.thisNode(&l_status);
    MFnDependencyNode k_dep{l_object};
    if (k_dep.typeName(&l_status) == qlSolverShape) {
      break;
    }
  }
  DOODLE_CHICK(!l_object.isNull(), doodle_error{"没有找到qlSolver解算核心"s});
  return l_object;
}

MDagPath qcloth_shape::ql_cloth_shape() const { return get_dag_path(obj); }

MDagPath qcloth_shape::cloth_mesh() const {
  MStatus l_s{};
  MObject l_mesh{};
  /// \brief 获得组件点上下文
  DOODLE_LOG_INFO(fmt::format("使用q布料节点 {}", get_node_full_name(obj)));
  auto l_shape = maya_plug::get_shape(obj);
  auto l_plug  = get_plug(l_shape, "outputMesh");

  /// 寻找高模的皮肤簇
  for (MItDependencyGraph i{l_plug, MFn::kMesh, MItDependencyGraph::Direction::kDownstream}; !i.isDone(); i.next()) {
    l_mesh = i.currentItem(&l_s);
    DOODLE_MAYA_CHICK(l_s);
    break;
  }

  DOODLE_CHICK(!l_mesh.isNull(), doodle_error{"没有找到布料模型节点"s});
  auto l_path = get_dag_path(l_mesh);
  DOODLE_LOG_INFO("找到布料节点 {}", l_path);
  return l_path;
}

void qcloth_shape::sim_cloth() const {
  DOODLE_CHICK(!obj.isNull(), doodle_error{"空组件"});
  MStatus k_s{};
  auto k_plug = get_plug(obj, "outputMesh");
  /// \brief 使用这种方式评估网格
  k_plug.asMObject(&k_s).isNull();
  maya_chick(k_s);
}
void qcloth_shape::add_field(const entt::handle& in_handle) const {
  auto l_f = in_handle.get<reference_file>().get_field_dag();
  if (l_f) {
    auto l_mesh = cloth_mesh();
    DOODLE_LOG_INFO("开始设置解算布料 {} 关联的风场", l_mesh);
    MStatus l_status{};
    MSelectionList l_select_list{};

    MItMeshVertex l_it{l_mesh, MObject::kNullObj, &l_status};
    DOODLE_MAYA_CHICK(l_status);
    for (; !l_it.isDone(); l_it.next()) {
      auto l_obj = l_it.currentItem(&l_status);
      DOODLE_MAYA_CHICK(l_status);
      l_status = l_select_list.add(l_mesh, l_obj);
      DOODLE_MAYA_CHICK(l_status);
    }
    /// @brief  这里必须要最后加入
    l_status = l_select_list.add(*l_f);
    DOODLE_MAYA_CHICK(l_status);
    MGlobal::setActiveSelectionList(l_select_list);
    DOODLE_LOG_INFO("设置布料风场 {}", *l_f);
    l_status = MGlobal::executeCommand(d_str{"qlConnectField;"});
  }
}
void qcloth_shape::add_collision(const entt::handle& in_handle) const {
  MStatus k_s{};
  auto l_item = in_handle.get<reference_file>().get_collision_model();
  if (l_item.isEmpty()) return;

  k_s = l_item.add(get_solver(), true);
  maya_chick(k_s);
  k_s = MGlobal::setActiveSelectionList(l_item);
  maya_chick(k_s);
  k_s = MGlobal::executeCommand(d_str{"qlCreateCollider;"});
  maya_chick(k_s);
}
void qcloth_shape::rest(const entt::handle& in_handle) const {
  MSelectionList l_list{};
  auto l_simple_module_proxy_ = std::string{"_proxy"};
  auto l_proxy_               = "_cloth_proxy"s;
  maya_chick(l_list.add(obj));

  auto k_plug = get_plug(obj, "outputMesh");
  MStatus l_s{};
  MItDependencyGraph l_it{
      k_plug,
      MFn::kMesh,
      MItDependencyGraph::Direction::kDownstream,
      MItDependencyGraph::kDepthFirst,
      MItDependencyGraph::kNodeLevel,
      &l_s
  };
  maya_chick(l_s);

  auto l_path = get_transform(l_it.currentItem());
  auto l_name = get_node_name(l_path);
  boost::replace_last(l_name, l_proxy_, l_simple_module_proxy_);

  DOODLE_LOG_INFO("开始寻找布料对应的初识姿势 {} -> {}", get_node_name(l_path), l_name);
  maya_chick(l_list.add(conv::to_ms(l_name)));
  maya_chick(MGlobal::setActiveSelectionList(l_list));
  auto l_status = MGlobal::executeCommand(d_str{"qlUpdateInitialPose;"});
  if (!l_status) {
    DOODLE_LOG_WARN("布料对应的初识姿势 {} 设置出错", l_name);
  }
}
MObject qcloth_shape::get_solver() const {
  MStatus l_status{};
  MObject l_object{};
  auto l_root = obj;
  for (MItDependencyGraph l_it_dependency_graph{
           l_root, MFn::kPluginLocatorNode, MItDependencyGraph::kUpstream, MItDependencyGraph::kDepthFirst,
           MItDependencyGraph::kNodeLevel, nullptr
       };
       !l_it_dependency_graph.isDone(); l_it_dependency_graph.next()) {
    l_object = l_it_dependency_graph.currentItem(&l_status);
    const MFnDependencyNode k_dep{l_object};
    if (k_dep.typeName(&l_status) == qlSolverShape) {
      return l_object;
    }
  }
  DOODLE_CHICK(!l_object.isNull(), doodle_error{"没有找到qlSolver解算核心"s});
  return l_object;
}

MDagPath qcloth_shape::get_shape() const {
  DOODLE_LOG_INFO(fmt::format("使用q布料节点 {}", get_node_full_name(obj)));
  return maya_plug::get_dag_path(obj);
}

std::string qcloth_shape::get_namespace() const { return m_namespace::get_namespace_from_name(get_node_name(obj)); };

void qcloth_shape::cover_cloth_attr(const entt::handle& in_handle) const {
  reference_file l_file = in_handle.get<reference_file>();

  auto l_node           = l_file.get_file_info_node();
  auto l_ql_core        = get_ql_solver();
  if (l_ql_core.isNull()) {
    default_logger_raw()->warn("{} 没有找到解算核心", get_node_full_name(obj));
    return;
  }

  if (!get_attribute<std::int32_t>(l_node, "sim_override")) {
    default_logger_raw()->warn("{} 没有开启覆盖模式", get_node_full_name(obj));
    return;
  }
  set_attribute(l_ql_core, "simpleSubsampling", get_attribute<std::int32_t>(l_node, "simple_subsampling"));
  set_attribute(l_ql_core, "frameSamples", get_attribute<std::int32_t>(l_node, "frame_samples"));
  set_attribute(l_ql_core, "timeScale", get_attribute<std::double_t>(l_node, "time_scale"));
  set_attribute(l_ql_core, "lengthScale", get_attribute<std::double_t>(l_node, "length_scale"));
  set_attribute(l_ql_core, "maxCGIteration", get_attribute<std::int32_t>(l_node, "max_cg_iteration"));
  set_attribute(l_ql_core, "cgAccuracy", get_attribute<std::int32_t>(l_node, "cg_accuracy"));
  set_attribute(l_ql_core, "gravity0", get_attribute<std::double_t>(l_node, "gravityx"));
  set_attribute(l_ql_core, "gravity1", get_attribute<std::double_t>(l_node, "gravityy"));
  set_attribute(l_ql_core, "gravity2", get_attribute<std::double_t>(l_node, "gravityz"));
}

void qcloth_shape::set_cache_folder_read_only(const entt::handle& in_handle) const {
  // std::string k_namespace = in_handle.get<reference_file>().get_namespace();
  // std::string k_node_name = m_namespace::strip_namespace_from_name(get_node_full_name(obj));
  auto l_cache_folder = conv::to_s(get_attribute<MString>(obj, "cacheFolder"));
  // auto k_cache            = get_plug(obj, "cacheFolder");
  // auto k_file_name    = maya_file_io::get_current_path();
  // FSys::path l_string = fmt::format("cache/{}/{}/{}", k_file_name.stem().generic_string(), k_namespace, k_node_name);
  // l_string /= in_path;
  // DOODLE_LOG_INFO("设置缓存路径 {}", l_string);
  auto k_path         = maya_file_io::work_path(l_cache_folder);
  DOODLE_LOG_WARN("发现缓存路径 {}", k_path);
  if (!FSys::exists(k_path))
    throw_error(maya_enum::maya_error_t::cache_path_error, fmt::format("缓存路径 {} 不存在", k_path));
  // if (need_clear && FSys::exists(k_path)) {
  // DOODLE_LOG_INFO("发现缓存目录, 主动删除 {}", k_path);
  // FSys::remove_all(k_path);
  // }
  // FSys::create_directories(k_path);
  // set_attribute(obj, "cacheFolder", l_string.generic_string());
  // set_attribute(obj, "cacheName", k_node_name);
  set_attribute(obj, "readOnly", true);
}

}  // namespace doodle::maya_plug
