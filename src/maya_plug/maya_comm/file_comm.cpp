//
// Created by TD on 2021/12/15.
//

#include "file_comm.h"
#include <maya/MArgParser.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_core/metadata/metadata.h>

#include <maya_plug/data/maya_file_io.h>

#define doodle_filepath "-fp"
#define doodle_filepath_long "-filepath"

namespace doodle::maya_plug {
MSyntax details::comm_file_save_syntax() {
  MSyntax syntax{};
  syntax.addFlag(doodle_filepath, doodle_filepath_long, MSyntax::kString);
  return syntax;
}

MStatus comm_file_save::doIt(const MArgList& in_arg) {
  MStatus k_s;
  MArgParser k_prase{syntax(), in_arg};

  if (k_prase.isFlagSet(doodle_filepath, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    auto k_path = k_prase.flagArgumentString(doodle_filepath, 0, &k_s);
    DOODLE_MAYA_CHICK(k_s);
    DOODLE_LOG_INFO("获得传入路径: {}", k_path);
    maya_file_io::save_file(d_str{k_path}.str());
  } else {
    maya_file_io::save_file(maya_file_io::work_path("fbx") / maya_file_io::get_current_path().stem() / maya_file_io::get_current_path().filename());
  }
  return k_s;
}
}  // namespace doodle::maya_plug
