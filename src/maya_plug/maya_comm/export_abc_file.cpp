//
// Created by TD on 24-5-10.
//

#include "export_abc_file.h"

#include <maya_plug/abc/alembic_archive_out.h>
#include <maya_plug/data/dagpath_cmp.h>
#include <maya_plug/fmt/fmt_warp.h>

#include "maya/MAnimControl.h"
#include "maya/MArgDatabase.h"
#include "maya/MDagPath.h"
#include "maya/MItDag.h"
#include "maya/MItSelectionList.h"
#include "maya/MSelectionList.h"
#include "maya/MSyntax.h"

namespace doodle::maya_plug {
MSyntax export_abc_file_syntax() {
  MSyntax syntax;
  syntax.addFlag("-f", "-file_path", MSyntax::kString);
  syntax.addFlag("-s", "-start", MSyntax::kLong);
  syntax.addFlag("-e", "-end", MSyntax::kLong);
  syntax.setObjectType(MSyntax::kSelectionList);
  syntax.useSelectionAsDefault(true);
  return syntax;
}
MStatus export_abc_file::doIt(const MArgList &in_arg) {
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
    MDagPath dag_path{};
    status = it_list.getDagPath(dag_path);
    maya_chick(status);

    if (dag_path.hasFn(MFn::kMesh)) {
      dag_path_list.push_back(dag_path);
    }
    MItDag it_dag{};
    for (it_dag.reset(dag_path); !it_dag.isDone(); it_dag.next()) {
      MDagPath child_path{};
      status = it_dag.getPath(child_path);
      maya_chick(status);
      if (child_path.hasFn(MFn::kMesh)) {
        dag_path_list.push_back(child_path);
      }
    }
  }
  if (dag_path_list.empty()) {
    return MS::kFailure;
  }
  std::ranges::sort(dag_path_list, details::cmp_dag{});
  dag_path_list.erase(std::unique(dag_path_list.begin(), dag_path_list.end()), dag_path_list.end());
  try {
    alembic::archive_out archive_out{file_name, dag_path_list, begin_time, end_time};
    for (auto i = begin_time; i <= end_time; ++i) {
      MAnimControl::setCurrentTime(i);
      archive_out.write();
    }
  } catch (std::exception const &e) {
    default_logger_raw()->error("export_abc_file::doIt: {}", e.what());
    return MS::kFailure;
  }
  return MS::kSuccess;
}
}  // namespace doodle::maya_plug