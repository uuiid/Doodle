//
// Created by TD on 2024/1/11.
//

#include "file_info_edit.h"

#include <maya_plug/node/files_info.h>

#include <maya/MArgDatabase.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnReference.h>
#include <maya/MItDependencyNodes.h>

namespace doodle::maya_plug {

MSyntax file_info_edit_syntax() {
  MSyntax l_syntax{};
  //  l_syntax.addFlag("-rf", "-reference_file", MSyntax::kString);
  //  l_syntax.addFlag("-rnp", "-reference_file_namespace", MSyntax::kString);
  //  l_syntax.addFlag("-rp", "-reference_file_path", MSyntax::kString);
  //  l_syntax.addFlag("-is", "-is_solve", MSyntax::kBoolean);
  //  l_syntax.addFlag("-co", "-collision_objects", MSyntax::kString);
  //  l_syntax.addFlag("-wf", "-wind_fields", MSyntax::kString);
  // 刷新
  l_syntax.addFlag("-r", "-refresh", MSyntax::kNoArg);
  // 强制
  l_syntax.addFlag("-f", "-force", MSyntax::kNoArg);

  return l_syntax;
}

MStatus file_info_edit::doIt(const MArgList &in_list) {
  MStatus l_status{};
  MArgDatabase l_arg_data{syntax(), in_list, &l_status};
  maya_chick(l_status);

  if (l_arg_data.isFlagSet("-f")) {
    is_force = true;
  }

  if (l_arg_data.isFlagSet("-r")) {
    l_status = todo();
  }

  return redoIt();
}

MStatus file_info_edit::redoIt() {
  MStatus l_status{};
  if (has_node() && !is_force) {
    displayError("has node, use -f to force");
    return MS::kSuccess;
  }
  if (is_force) {
    l_status = delete_node();
    maya_chick(l_status);
  }
  for (MItDependencyNodes l_it{MFn::kReference, &l_status}; !l_it.isDone(); l_it.next()) {
    MObject l_node = dg_modifier_.createNode(doodle_file_info::doodle_id, &l_status);
    maya_chick(l_status);
    MFnDependencyNode l_fn_node{l_node, &l_status};
    maya_chick(l_status);

    maya_chick(dg_modifier_.connect(get_plug(l_it.thisNode(), "message"), get_plug(l_node, "reference_file")));
    MFnReference l_fn_ref{l_it.thisNode(), &l_status};
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
  return l_status;
}
MStatus file_info_edit::undoIt() { return dg_modifier_.undoIt(); }

bool file_info_edit::isUndoable() const { return true; }

MStatus file_info_edit::delete_node() {
  MStatus l_status{};
  for (MItDependencyNodes l_it{MFn::kPluginDependNode, &l_status}; !l_it.isDone(); l_it.next()) {
    if (l_it.thisNode().apiTypeStr() == doodle_file_info::node_name.data()) {
      maya_chick(dg_modifier_.deleteNode(l_it.thisNode(&l_status)));
      maya_chick(l_status);
    }
  }
  return l_status;
}

bool file_info_edit::has_node() const {
  MStatus l_status{};
  for (MItDependencyNodes l_it{MFn::kPluginDependNode, &l_status}; !l_it.isDone(); l_it.next()) {
    if (l_it.thisNode().apiTypeStr() == doodle_file_info::node_name.data()) return true;
  }
  return false;
}

}  // namespace doodle::maya_plug
