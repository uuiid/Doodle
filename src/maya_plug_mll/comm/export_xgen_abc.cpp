//
// Created by TD on 2025/9/3.
//

#include "export_xgen_abc.h"

#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MFn.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>
#include <maya/MItSelectionList.h>
#include <maya/MSelectionList.h>
#include <xgen/src/sggeom/SgVec3T.h>
#include <xgen/src/sggeom/SgXform3T.h>
#include <xgen/src/xgcore/XgCreator.h>
#include <xgen/src/xgcore/XgDescription.h>
#include <xgen/src/xgcore/XgGenerator.h>
#include <xgen/src/xgcore/XgPalette.h>
#include <xgen/src/xgcore/XgPatch.h>
#include <xgen/src/xgcore/XgPrimitive.h>
#include <xgen/src/xgcore/XgRenderer.h>
#include <xgen/src/xgcore/XgUtil.h>
#include <xgen/src/xgprimitive/XgArchivePrimitive.h>
#include <xgen/src/xgprimitive/XgCardPrimitive.h>
#include <xgen/src/xgprimitive/XgSpherePrimitive.h>
#include <xgen/src/xgprimitive/XgSplinePrimitive.h>
namespace doodle::maya_plug {
struct xgen_abc_export::impl {
  std::vector<XgPalette*> palette_v{};
};
xgen_abc_export::xgen_abc_export() : p_i{std::make_unique<impl>()} {}
xgen_abc_export::~xgen_abc_export() = default;
MSyntax xgen_abc_export_syntax() {
  MSyntax syntax;
  syntax.addFlag("-f", "-file_path", MSyntax::kString);
  syntax.addFlag("-s", "-start", MSyntax::kLong);
  syntax.addFlag("-e", "-end", MSyntax::kLong);
  syntax.setObjectType(MSyntax::kSelectionList);
  syntax.useSelectionAsDefault(true);
  return syntax;
}
MStatus xgen_abc_export::doIt(const MArgList& in_arg) {
  parse_args(in_arg);
  return redoIt();
}
MStatus xgen_abc_export::redoIt() {
  for (auto l_ptr : p_i->palette_v) {
    for (auto i = 0; i < l_ptr->numDescriptions(); ++i) {
      auto l_des       = l_ptr->description(i);
      auto l_previewer = l_des->activePreviewer();
      auto l_generator = l_des->activeGenerator();
      auto l_primitive = l_des->activePrimitive();
      auto l_path      = l_primitive->cPatch();
      auto l_geom      = l_primitive->cGeom();
      auto l_str = fmt::format("previewer {}", l_previewer->totalEmitCount());
      displayInfo(l_str.c_str());
      // if (l_primitive->typeName() == "SplinePrimitive") {
      //   const auto l_primitive_spline = dynamic_cast<XgSplinePrimitive*>(l_primitive);
      //   safevector<SgVec3d> l_geom_v{};
      //   SgCurveUtil::mkPolyLine(false, l_primitive_spline->getGeom(), l_geom_v);
      //   auto l_size = l_geom_v.size();
      //   displayInfo(
      //       fmt::format(
      //           "{} geom num {} path {} , spline geom num {}", l_des->name(), l_geom.size(), l_path->name(), l_size
      //       )
      //           .c_str()
      //   );
      // }
    }
  }

  return MStatus::kSuccess;
}

void xgen_abc_export::parse_args(const MArgList& in_arg) {
  MStatus status;
  MArgDatabase const arg_data{syntax(), in_arg, &status};
  maya_chick(status);
  MSelectionList list{};
  maya_chick(arg_data.getObjects(list));
  auto begin_time = arg_data.isFlagSet("-s")
                        ? MTime{boost::numeric_cast<std::double_t>(arg_data.flagArgumentInt("-s", 0)), MTime::uiUnit()}
                        : MAnimControl::minTime();
  auto end_time   = arg_data.isFlagSet("-e")
                        ? MTime{boost::numeric_cast<std::double_t>(arg_data.flagArgumentInt("-e", 0)), MTime::uiUnit()}
                        : MAnimControl::maxTime();
  auto file_name  = arg_data.isFlagSet("-f") ? FSys::path{conv::to_s(arg_data.flagArgumentString("-f", 0))}
                                             : FSys::get_cache_path() / "default.abc";

  default_logger_raw()->info(
      "export_abc_file::doIt: file_name: {}, begin_time: {}, end_time: {}", file_name, begin_time, end_time
  );

  MItSelectionList it_list{list, MFn::kDagNode, &status};
  maya_chick(status);
  std::vector<MDagPath> dag_path_list{};
  for (; !it_list.isDone(); it_list.next()) {
    MDagPath l_dag_path{};
    status = it_list.getDagPath(l_dag_path);
    maya_chick(status);
    // displayInfo(l_dag_path.fullPathName());
    if (auto l_ptr = XgPalette::palette(get_node_name(l_dag_path)); l_ptr) p_i->palette_v.emplace_back(l_ptr);
  }
}

MStatus xgen_abc_export::undoIt() { return MStatus::kSuccess; }

}  // namespace doodle::maya_plug
