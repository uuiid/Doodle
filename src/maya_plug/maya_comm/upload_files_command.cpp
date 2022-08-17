//
// Created by TD on 2022/5/12.
//

#include "upload_files_command.h"

#include <doodle_core/metadata/export_file_info.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya/MArgParser.h>
namespace doodle::maya_plug {

namespace {
constexpr auto export_cam_clear      = "-c";
constexpr auto export_cam_clear_long = "-clear";
}  // namespace

MSyntax upload_files_command_ns::syntax() {
  MSyntax syntax{};
  syntax.addFlag(export_cam_clear, export_cam_clear_long, MSyntax::kBoolean);

  return syntax;
}
MStatus upload_files_command::doIt(const MArgList& in_list) {
  bool is_clear{false};

  MStatus k_s{};
  MArgParser k_prase{syntax(), in_list, &k_s};
  DOODLE_MAYA_CHICK(k_s);

  if (k_prase.isFlagSet(export_cam_clear, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    k_s = k_prase.getFlagArgument(export_cam_clear, 0, is_clear);
    DOODLE_MAYA_CHICK(k_s);
  }
  if (!is_clear) {
    for (auto&& [e, f] : g_reg()->view<export_file_info>().each()) {
      maya_file_io::upload_file(f.file_path, f.upload_path_);
      f.file_path = f.upload_path_;
      f.write_file(make_handle(e));
    }
  }
  auto l_v = g_reg()->view<export_file_info>();
  g_reg()->destroy(l_v.begin(), l_v.end());
  return k_s;
}

}  // namespace doodle::maya_plug
