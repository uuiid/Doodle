#include "dna_calib_import.h"

#include <maya/MArgDatabase.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>

namespace doodle::maya_plug {
MSyntax dna_calib_import_syntax() {
  MSyntax l_syntax{};
  l_syntax.addFlag("fp", "file_path", MSyntax::kString);
  l_syntax.setObjectType(MSyntax::kSelectionList);
  l_syntax.useSelectionAsDefault(true);
  return l_syntax;
}

class dna_calib_import::impl {
 public:
  FSys::path file_path;
};
dna_calib_import::dna_calib_import() : p_i(std::make_unique<impl>()) {}

MStatus dna_calib_import::get_arg(const MArgList& in_arg) {
  MStatus l_status{};
  MArgDatabase const l_arg_data{syntax(), in_arg, &l_status};
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  if (l_arg_data.isFlagSet("fp", &l_status)) {
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    MString l_file_path{};
    l_status = l_arg_data.getFlagArgument("fp", 0, l_file_path);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    p_i->file_path = conv::to_s(l_file_path);
  }
  return MS::kSuccess;
}

MStatus dna_calib_import::doIt(const MArgList& in_list) {
  get_arg(in_list);
  MSelectionList l_list{};
  MStatus l_status{};
  MArgDatabase const l_arg_data{syntax(), in_list, &l_status};
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  l_status = l_arg_data.getObjects(l_list);
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  for (unsigned int i = 0; i < l_list.length(); ++i) {
    MDagPath l_dag_path{};
    l_status = l_list.getDagPath(i, l_dag_path);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    MObject l_node_obj = l_dag_path.node(&l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    if (!l_node_obj.hasFn(MFn::kDependencyNode)) continue;
    MFnDependencyNode l_fn_node{l_node_obj, &l_status};
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    if (l_fn_node.typeId() != dna_calib_node::doodle_id) continue;
    MPlug l_plug = l_fn_node.findPlug(dna_calib_node::dna_file_path, true, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    l_status = l_plug.setValue(conv::to_ms(p_i->file_path));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  }
}

}  // namespace doodle::maya_plug