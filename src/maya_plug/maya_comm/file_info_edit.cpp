//
// Created by TD on 2024/1/11.
//

#include "file_info_edit.h"

#include <maya_plug/node/files_info.h>

#include <maya/MArgDatabase.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnReference.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItSelectionList.h>

namespace doodle::maya_plug {
file_info_edit::file_info_edit()  = default;
file_info_edit::~file_info_edit() = default;

MSyntax file_info_edit_syntax() {
  MSyntax l_syntax{};
  //  l_syntax.addFlag("-rf", "-reference_file", MSyntax::kString);
  //  l_syntax.addFlag("-rnp", "-reference_file_namespace", MSyntax::kString);
  //  l_syntax.addFlag("-rp", "-reference_file_path", MSyntax::kString);
  //  l_syntax.addFlag("-is", "-is_solve", MSyntax::kBoolean);
  //  l_syntax.addFlag("-co", "-collision_objects", MSyntax::kString);
  //  l_syntax.addFlag("-wf", "-wind_fields", MSyntax::kString);

  // 强制
  l_syntax.addFlag("-f", "-force", MSyntax::kNoArg);
  // 节点
  l_syntax.addFlag("-n", "-node", MSyntax::kString);
  // 添加碰撞
  l_syntax.addFlag("-ac", "-add_collision", MSyntax::kSelectionItem);

  l_syntax.useSelectionAsDefault(true);

  return l_syntax;
}

MStatus file_info_edit::doIt(const MArgList &in_list) {
  MStatus l_status{};
  MArgDatabase l_arg_data{syntax(), in_list, &l_status};
  maya_chick(l_status);

  if (l_arg_data.isFlagSet("-f")) {
    is_force = true;
  }
  if (l_arg_data.isFlagSet("-n")) {
    MString l_node_name{};
    l_arg_data.getFlagArgument("-n", 0, l_node_name);
    MSelectionList l_selection_list{};
    l_status = l_selection_list.add(l_node_name);
    maya_chick(l_status);
    l_status = l_selection_list.getDependNode(0, p_current_node);
    maya_chick(l_status);
  }
  if (l_arg_data.isFlagSet("-ac")) {
    maya_chick(l_arg_data.getObjects(p_selection_list));
  }
  return redoIt();
}

MStatus file_info_edit::redoIt() {
  MStatus l_status{};
  if (p_selection_list.length() != 0) {
    l_status = add_collision();
  } else {
    l_status = create_node();
  }
  return l_status;
}

MStatus file_info_edit::add_collision() {
  MStatus l_status{};
  MFnDependencyNode l_fn_node{p_current_node, &l_status};
  maya_chick(l_status);

  // 清除所有的连接
  {
    auto l_plug = l_fn_node.findPlug(doodle_file_info::collision_objects, true);
    for (auto i = 0; i < l_plug.numConnectedElements(); ++i) {
      auto l_connected_plug = l_plug.connectionByPhysicalIndex(i, &l_status);
      maya_chick(l_status);
      maya_chick(dg_modifier_.disconnect(l_connected_plug, l_plug));
    }
  }
  // 重新连接
  for (MItSelectionList l_it{p_selection_list}; !l_it.isDone(); l_it.next()) {
    MObject l_obj{};
    l_it.getDependNode(l_obj);
    MFnDependencyNode l_fn_node_2{l_obj, &l_status};
    maya_chick(l_status);
    if (l_fn_node_2.typeId() == MFn::kMesh) {
      maya_chick(dg_modifier_.connect(get_plug(l_obj, "message"), get_plug(p_current_node, "collision_objects")));
    }
  }
  maya_chick(dg_modifier_.doIt());
  return l_status;
}

MStatus file_info_edit::create_node() {
  MStatus l_status{};
  {  // 创建节点
    if (has_node() && !is_force) {
      displayError("has node, use -f to force");
      return MS::kSuccess;
    }
    if (is_force) {
      l_status = delete_node();
      maya_chick(l_status);
    }
    for (MItDependencyNodes l_it{MFn::kReference, &l_status}; !l_it.isDone(); l_it.next()) {
      MFnReference l_fn_ref{l_it.thisNode(), &l_status};
      if (!l_fn_ref.isLoaded()) continue;

      MObject l_node = dg_modifier_.createNode(doodle_file_info::doodle_id, &l_status);
      maya_chick(l_status);
      MFnDependencyNode l_fn_node{l_node, &l_status};
      maya_chick(l_status);

      maya_chick(dg_modifier_.connect(get_plug(l_it.thisNode(), "message"), get_plug(l_node, "reference_file")));
      dg_modifier_.newPlugValueString(
          get_plug(l_node, "reference_file_path"), l_fn_ref.fileName(false, false, false, &l_status)
      );
      maya_chick(l_status);
      dg_modifier_.newPlugValueString(
          get_plug(l_node, "reference_file_namespace"), l_fn_ref.associatedNamespace(false, &l_status)
      );
      maya_chick(l_status);
      maya_chick(dg_modifier_.doIt());
    }
  }
  return l_status;
}

MStatus file_info_edit::undoIt() { return dg_modifier_.undoIt(); }

bool file_info_edit::isUndoable() const { return true; }

MStatus file_info_edit::delete_node() {
  MStatus l_status{};
  for (MItDependencyNodes l_it{MFn::kPluginDependNode, &l_status}; !l_it.isDone(); l_it.next()) {
    MFnDependencyNode l_fn_node{l_it.thisNode(), &l_status};
    maya_chick(l_status);
    if (l_fn_node.typeId() == doodle_file_info::doodle_id) {
      maya_chick(dg_modifier_.deleteNode(l_it.thisNode(&l_status)));
      maya_chick(l_status);
    }
  }
  return l_status;
}

bool file_info_edit::has_node() const {
  MStatus l_status{};
  for (MItDependencyNodes l_it{MFn::kPluginDependNode, &l_status}; !l_it.isDone(); l_it.next()) {
    MFnDependencyNode l_fn_node{l_it.thisNode(), &l_status};
    maya_chick(l_status);
    if (l_fn_node.typeId() == doodle_file_info::doodle_id) return true;
  }
  return false;
}

}  // namespace doodle::maya_plug
