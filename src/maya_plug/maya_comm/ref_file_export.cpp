//
// Created by td_main on 2023/6/7.
//

#include "ref_file_export.h"


#include <maya_plug/data/export_file_fbx.h>
#include <maya_plug/data/qcloth_factory.h>
#include <maya_plug/data/reference_file.h>

#include "exception/exception.h"
#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgParser.h>
#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MNamespace.h>
#include <maya/MSelectionList.h>
namespace doodle {
namespace maya_plug {
MSyntax ref_file_export_syntax() {
  MSyntax l_syntax{};
  l_syntax.addArg(MSyntax::MArgType::kSelectionItem);
  l_syntax.useSelectionAsDefault(true);
  l_syntax.setObjectType(MSyntax::MObjectFormat::kSelectionList);
  return l_syntax;
}
MStatus ref_file_export::doIt(const MArgList& in_list) {
  MStatus l_status{};
  MArgDatabase k_prase{syntax(), in_list, &l_status};
  MString l_name_s{};

  MSelectionList l_list{};
  maya_chick(k_prase.getObjects(l_list));

  auto l_refs = reference_file_factory{}.create_ref(l_list);
  if (l_refs.empty()) return {};

  auto l_cloth = qcloth_factory{}.create_cloth();
  DOODLE_LOG_INFO("开始导出abc");

  auto l_gen               = std::make_shared<reference_file_ns::generate_abc_file_path>();

  const MTime l_begin_time = MTime{boost::numeric_cast<std::double_t>(1001), MTime::uiUnit()};
  const MTime l_end_time   = MAnimControl::maxTime();
  l_gen->begin_end_time    = std::make_pair(l_begin_time, l_end_time);

  export_file_fbx l_ex_fbx{};
  ranges::for_each(l_refs, [&](reference_file& in_handle) {
    l_gen->set_fbx_path(true);
    l_ex_fbx.export_sim(in_handle, l_gen, l_cloth);
  });

  return MStatus{};
}
}  // namespace maya_plug
}  // namespace doodle