#include "dna_calib_import.h"

#include "doodle_core/exception/exception.h"

#include <maya_plug/node/dna_calib_node.h>

#include "data/maya_conv_str.h"
#include "data/maya_tool.h"
#include <dnacalib/DNACalib.h>
#include <fmt/format.h>
#include <maya/MArgDatabase.h>
#include <maya/MFn.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>
#include <pma/ScopedPtr.h>

namespace doodle::maya_plug {
MSyntax dna_calib_import_syntax() {
  MSyntax l_syntax{};
  CHECK_MSTATUS(l_syntax.setObjectType(MSyntax::kSelectionList, 1, 1));
  l_syntax.useSelectionAsDefault(true);
  return l_syntax;
}

class dna_calib_import::impl {
 public:
  FSys::path file_path;
  MObject dna_node_obj{};

  void import_dna_calib() {
    auto l_dna_file = dnac::makeScoped<dnac::FileStream>(
        file_path.generic_string().data(), dnac::FileStream::AccessMode::ReadWrite, dnac::FileStream::OpenMode::Binary
    );
    auto l_render = dnac::makeScoped<dnac::BinaryStreamReader>(l_dna_file.get());
    l_render->read();
    DOODLE_CHICK(dnac::Status::isOk(), "读取dna文件失败: {}", file_path);

    auto l_dna_render = dnac::makeScoped<dnac::DNACalibDNAReader>(l_render.get());
  }
};
dna_calib_import::dna_calib_import() : p_i(std::make_unique<impl>()) {}

MStatus dna_calib_import::get_arg(const MArgList& in_arg) {
  MStatus l_status{};
  MArgDatabase const l_arg_data{syntax(), in_arg, &l_status};
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  MSelectionList l_list{};
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_arg_data.getObjects(l_list));
  for (unsigned int i = 0; i < l_list.length(); ++i) {
    MObject l_node_obj{};
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_list.getDependNode(i, l_node_obj));
    if (!l_node_obj.hasFn(MFn::kDependencyNode)) continue;
    MFnDependencyNode l_fn_node{l_node_obj, &l_status};
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    if (l_fn_node.typeId() != dna_calib_node::doodle_id) continue;
    p_i->dna_node_obj = l_node_obj;
    break;
  }
  return MS::kSuccess;
}

MStatus dna_calib_import::doIt(const MArgList& in_list) {
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(get_arg(in_list));

  if (p_i->dna_node_obj.isNull())
    return displayError(conv::to_ms(fmt::format("未选择dna_calib_node节点"))), MS::kFailure;

  // 找到属性
  MStatus l_status{};
  MFnDependencyNode l_fn_node{p_i->dna_node_obj, &l_status};
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  auto l_plug_file_path = l_fn_node.findPlug(dna_calib_node::dna_file_path, false, &l_status);
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  MString l_file_path_ms{};
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_plug_file_path.getValue(l_file_path_ms));
  p_i->file_path = conv::to_s(l_file_path_ms);
  if (p_i->file_path.empty() || !FSys::exists(p_i->file_path))
    return displayError(conv::to_ms(fmt::format("dna文件不存在: {}", p_i->file_path.generic_string()))), MS::kFailure;
  // 读取文件

  return MS::kSuccess;
}

}  // namespace doodle::maya_plug