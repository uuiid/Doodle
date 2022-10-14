//
// Created by TD on 2021/12/15.
//

#include "file_comm.h"

#include <doodle_core/metadata/metadata.h>

#include <maya_plug/data/maya_file_io.h>

#include <maya/MArgParser.h>

namespace doodle::maya_plug {

namespace comm_file_save_ns {

constexpr char file_path[]           = "-fp";
constexpr char file_path_long[]      = "-filepath";

constexpr char file_extension[]      = "-fe";
constexpr char file_extension_long[] = "-fileextension";

}  // namespace comm_file_save_ns

MSyntax details::comm_file_save_syntax() {
  MSyntax syntax{};
  syntax.addFlag(comm_file_save_ns::file_path, comm_file_save_ns::file_path_long, MSyntax::kString);
  syntax.addFlag(comm_file_save_ns::file_extension, comm_file_save_ns::file_extension_long, MSyntax::kString);
  return syntax;
}

MStatus comm_file_save::doIt(const MArgList& in_arg) {
  MStatus k_s;
  MArgParser k_prase{syntax(), in_arg};

  auto l_path = maya_file_io::work_path("fbx") / maya_file_io::get_current_path().stem() /
                maya_file_io::get_current_path().filename();

  if (k_prase.isFlagSet(comm_file_save_ns::file_path, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    auto k_path = k_prase.flagArgumentString(comm_file_save_ns::file_path, 0, &k_s);
    DOODLE_MAYA_CHICK(k_s);
    DOODLE_LOG_INFO("获得传入路径: {}", k_path);
    l_path = d_str{k_path}.str();
  }
  maya_file_io::save_file(l_path);
  return k_s;
}
}  // namespace doodle::maya_plug
