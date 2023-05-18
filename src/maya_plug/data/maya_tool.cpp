//
// Created by TD on 2021/12/16.
//

#include "maya_tool.h"

#include <maya_plug/main/maya_plug_fwd.h>

#include "exception/exception.h"
#include "maya/MApiNamespace.h"
#include "maya_conv_str.h"
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnSet.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItSelectionList.h>
#include <maya/MNamespace.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

namespace doodle::maya_plug {

MPlug get_plug(const MObject& in_node, const std::string& in_name) {
  in_node.isNull() ? throw_exception(doodle_error{"传入空节点寻找属性 {}"s, in_name}) : void();
  MStatus k_s{};
  MPlug l_plug{};

  MFnDependencyNode l_node{in_node, &k_s};
  DOODLE_MAYA_CHICK(k_s);
  try {
    l_plug = l_node.findPlug(d_str{in_name}, false, &k_s);

    if (!k_s) {  /// \brief 出错后再次寻找
      DOODLE_LOG_WARN(k_s.errorString());
      l_plug = l_node.findPlug(d_str{in_name}, true, &k_s);
      if (!k_s) {
        DOODLE_LOG_WARN(k_s.errorString());
      }
    }
  } catch (const std::runtime_error& error) {
    DOODLE_LOG_INFO("没有在这个节点中找到属性 {}", in_name);
  }
  if (!l_plug.isNull()) {
    return l_plug;
  }

  if (in_node.hasFn(MFn::kDagNode)) {
    MFnDagNode l_dag_node{in_node, &k_s};
    DOODLE_MAYA_CHICK(k_s);
    MDagPath l_path{};
    k_s = l_dag_node.getPath(l_path);
    DOODLE_MAYA_CHICK(k_s);
    try {
      k_s = l_path.extendToShape();
      DOODLE_MAYA_CHICK(k_s);
      MFnDagNode l_dag_node_shape{l_path, &k_s};
      l_plug = l_dag_node_shape.findPlug(d_str{in_name}, false, &k_s);

      if (!k_s) {
        DOODLE_LOG_WARN(k_s.errorString());
      }

      l_plug = l_dag_node_shape.findPlug(d_str{in_name}, true, &k_s);
      if (!k_s) {
        DOODLE_LOG_WARN(k_s.errorString());
      }

    } catch (const std::runtime_error& error) {
      DOODLE_LOG_INFO("节点下方没有 shape 形状节点, 不需要寻找形状节点");
    }
  }

  DOODLE_CHICK(!l_plug.isNull(), doodle_error{" {} 无法找到属性 {}", get_node_name(in_node), in_name});
  return l_plug;
}

bool is_intermediate(const MObject& in_node) {
  auto l_plug = get_plug(in_node, "intermediateObject"s);
  return !l_plug.isNull() && l_plug.asBool();
}
bool is_renderable(const MObject& in_node) {
  auto l_template = get_plug(in_node, "template"s);
  if (!l_template.isNull() && l_template.asBool()) {
    return false;
  }

  auto l_plug = get_plug(in_node, "visibility"s);
  if (!l_plug.isNull()) {
    if (!l_plug.asBool()) return false;

    MPlugArray l_plug_array{};
    MStatus l_s;
    l_plug.connectedTo(l_plug_array, true, false, &l_s);
    maya_chick(l_s);
    if (l_plug_array.length() == 0) return false;
  }
  auto l_lod = get_plug(in_node, "lodVisibility"s);
  return l_lod.isNull() || l_lod.asBool();
}

MObject get_shading_engine(const MObject& in_node) { return get_shading_engine(get_dag_path(in_node)); }
MObject get_shading_engine(const MDagPath& in_node) {
  MStatus k_s{};
  auto l_path = in_node;
  k_s         = l_path.extendToShape();
  DOODLE_MAYA_CHICK(k_s);
  MObject k_obj = l_path.node(&k_s);
  DOODLE_MAYA_CHICK(k_s);

  MObject obj{};
  for (MItDependencyGraph i{
           k_obj, MFn::Type::kShadingEngine, MItDependencyGraph::Direction::kDownstream,
           MItDependencyGraph::Traversal::kDepthFirst, MItDependencyGraph::Level::kNodeLevel, &k_s};
       !i.isDone(); i.next()) {
    DOODLE_MAYA_CHICK(k_s);
    obj = i.currentItem(&k_s);
    //    DOODLE_MAYA_CHICK(k_s);
    //    MFnDependencyNode k_node{};
    //    k_node.setObject(obj);
    //    DOODLE_LOG_INFO(fmt::format("找到节点 {}", d_str{k_node.name()}.str()));
    break;
  }
  DOODLE_CHICK(!obj.isNull(), doodle_error{"没有找到节点"});
  return obj;
}
MObject get_first_mesh(const MObject& in_node) {
  MStatus k_s{};
  MObject k_obj = in_node;
  MObject l_r{};

  for (MItDependencyGraph i{
           k_obj, MFn::Type::kMesh, MItDependencyGraph::Direction::kDownstream,
           MItDependencyGraph::Traversal::kDepthFirst, MItDependencyGraph::Level::kNodeLevel, &k_s};
       !i.isDone(); i.next()) {
    DOODLE_MAYA_CHICK(k_s);
    l_r = i.currentItem(&k_s);
    break;
  }
  DOODLE_CHICK(!l_r.isNull(), doodle_error{"没有在依赖网格中寻找到mesh节点"s});
  return l_r;
}
MObject get_shape(const MObject& in_object) {
  MStatus k_s{};
  MObject k_obj = in_object;
  MObject k_r{};
  if (k_obj.hasFn(MFn::kDagNode)) {
    MFnDagNode l_dag_node{k_obj, &k_s};
    DOODLE_MAYA_CHICK(k_s);
    MDagPath l_path{};
    k_s = l_dag_node.getPath(l_path);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_path.extendToShape();
    DOODLE_MAYA_CHICK(k_s);
    k_r = l_path.node(&k_s);
    DOODLE_MAYA_CHICK(k_s);
  }
  DOODLE_CHICK(!k_r.isNull(), doodle_error{"没有找到形状"s});
  return k_r;
}
MObject get_transform(const MObject& in_object) {
  MStatus k_s{};
  MObject k_obj = in_object;
  MObject k_r{};
  if (k_obj.hasFn(MFn::kDagNode)) {
    MFnDagNode l_dag_node{k_obj, &k_s};
    DOODLE_MAYA_CHICK(k_s);
    MDagPath l_path{};
    k_s = l_dag_node.getPath(l_path);
    DOODLE_MAYA_CHICK(k_s);
    k_r = l_path.transform(&k_s);
    DOODLE_MAYA_CHICK(k_s);
  }
  DOODLE_CHICK(!k_r.isNull(), doodle_error{"没有找到变换"s});
  return k_r;
}
void add_child(const MObject& in_praent, MObject& in_child) {
  add_child(get_dag_path(in_praent), get_dag_path(in_child));
}
void add_child(const MDagPath& in_praent, const MDagPath& in_child) {
  if (in_praent == in_child) return;

  MStatus k_s{};
  MFnDagNode k_node{in_praent, &k_s};
  DOODLE_MAYA_CHICK(k_s);
  auto l_node = in_child.node(&k_s);
  DOODLE_MAYA_CHICK(k_s);
  if (k_node.hasChild(l_node, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    k_node.removeChild(l_node);
  }
  k_s = k_node.addChild(l_node);
  DOODLE_MAYA_CHICK(k_s);
}
void add_mat(const MObject& in_object, MObject& in_ref_obj) {
  MStatus l_s{};
  auto k_mat = get_shading_engine(in_ref_obj);
  DOODLE_CHICK(k_mat.hasFn(MFn::kShadingEngine), doodle_error{"没有找到着色集"s});
  MFnSet l_set{k_mat, &l_s};
  DOODLE_MAYA_CHICK(l_s);
  l_set.addMember(in_object);
}
void maya_plug::copy_mat(const MDagPath& in_obj, MDagPath& in_ref_obj) {
  MStatus l_s{};
  auto k_mat = get_shading_engine(in_ref_obj.node());
  DOODLE_CHICK(k_mat.hasFn(MFn::kShadingEngine), doodle_error{"没有找到着色集"s});
  MFnSet l_set{k_mat, &l_s};
  DOODLE_MAYA_CHICK(l_s);
  l_set.addMember(in_obj);
}
std::string get_node_full_name(const MObject& in_obj) {
  if (in_obj.hasFn(MFn::kDagNode)) {
    return get_node_full_name(get_dag_path(in_obj));
  } else if (in_obj.hasFn(MFn::Type::kDependencyNode)) {
    MFnDependencyNode l_node{};
    DOODLE_MAYA_CHICK(l_node.setObject(in_obj));
    return d_str{l_node.absoluteName()};
  }
  return {};
}
std::string get_node_full_name(const MDagPath& in_obj) {
  MStatus l_s{};
  auto l_str = in_obj.fullPathName(&l_s);
  DOODLE_MAYA_CHICK(l_s);
  return d_str{l_str};
}
std::string get_node_name(const MObject& in_obj) {
  MFnDependencyNode l_node{};
  DOODLE_MAYA_CHICK(l_node.setObject(in_obj));
  MStatus l_s{};
  auto l_name = l_node.name(&l_s);
  DOODLE_MAYA_CHICK(l_s);
  return d_str{l_name};
}
std::string get_node_name(const MDagPath& in_obj) {
  MFnDependencyNode l_node{};
  MStatus l_s{};
  DOODLE_MAYA_CHICK(l_node.setObject(in_obj.node(&l_s)));
  DOODLE_MAYA_CHICK(l_s);
  auto l_name = l_node.name(&l_s);
  DOODLE_MAYA_CHICK(l_s);
  return d_str{l_name};
}
std::string set_node_name(const MObject& in_obj, const std::string& in_name) {
  MFnDependencyNode l_node{};
  DOODLE_MAYA_CHICK(l_node.setObject(in_obj));
  MStatus l_s{};
  l_node.setName(d_str{in_name}, true, &l_s);
  DOODLE_MAYA_CHICK(l_s);
  auto l_name = l_node.name(&l_s);
  DOODLE_MAYA_CHICK(l_s);
  return d_str{l_name};
}
MDagPath get_dag_path(const MObject& in_object) {
  MStatus l_s{};
  MFnDagNode l_node{in_object, &l_s};
  DOODLE_MAYA_CHICK(l_s);
  MDagPath l_path{};
  l_s = l_node.getPath(l_path);
  DOODLE_MAYA_CHICK(l_s);
  return l_path;
}
std::string get_node_name_strip_name_space(const MDagPath& in_obj) {
  MFnDependencyNode l_node{};
  MStatus l_s{};
  DOODLE_MAYA_CHICK(l_node.setObject(in_obj.node(&l_s)));
  DOODLE_MAYA_CHICK(l_s);
  auto l_name = l_node.name(&l_s);
  DOODLE_MAYA_CHICK(l_s);
  l_name = MNamespace::stripNamespaceFromName(l_name, &l_s);
  DOODLE_MAYA_CHICK(l_s);
  return d_str{l_name};
}

namespace comm_warp {
MDagPath marge_mesh(const MSelectionList& in_marge_obj, const std::string& in_marge_name) {
  MStatus k_s{};
  MSelectionList k_select{in_marge_obj};
  if (k_select.length(&k_s) > 1) {
    DOODLE_MAYA_CHICK(k_s);
    MStringArray k_mearge_names{};
    k_s = k_select.getSelectionStrings(k_mearge_names);
    DOODLE_MAYA_CHICK(k_s);
    std::vector<std::string> l_names;
    for (int l_i = 0; l_i < k_mearge_names.length(); ++l_i) {
      l_names.emplace_back(d_str{k_mearge_names[l_i]});
    }

    MStringArray k_r_s{};
    auto k_name = fmt::format("{}_export_abc", in_marge_name);
    std::string l_mel =
        fmt::format(R"(polyUnite -ch 1 -mergeUVSets 1 -centerPivot -name "{}" {};)", k_name, fmt::join(l_names, " "));
    DOODLE_LOG_INFO("开始合并网格体 {}", fmt::join(l_names, " "));
    k_s = MGlobal::executeCommand(d_str{l_mel}, k_r_s, true);
    DOODLE_MAYA_CHICK(k_s);

    k_select.clear();
    k_s = k_select.add(k_r_s[0], true);
    DOODLE_MAYA_CHICK(k_s);
  } else {
    DOODLE_LOG_INFO("只有一个网格体 不合并");
  }
  MDagPath k_mesh_path{};
  k_s = k_select.getDagPath(0, k_mesh_path);
  DOODLE_MAYA_CHICK(k_s);
  return get_dag_path(k_mesh_path.transform());
}
}  // namespace comm_warp

}  // namespace doodle::maya_plug
