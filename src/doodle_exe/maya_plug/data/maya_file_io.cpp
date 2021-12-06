//
// Created by TD on 2021/12/6.
//

#include "maya_file_io.h"

#include <maya/MFileIO.h>
#include <maya/MFileObject.h>
#include <maya_plug/maya_plug_fwd.h>
namespace doodle::maya_plug {

FSys::path maya_file_io::get_current_path() {
  auto k_s = MFileIO::currentFile();
  return {k_s.asUTF8()};
}
FSys::path maya_file_io::work_path(const FSys::path& in_path) {
  MFileObject k_obj{};

  k_obj.setRawName(d_str{in_path.generic_string()});
  k_obj.setResolveMethod(MFileObject::MFileResolveMethod::kNone);
  return k_obj.resolvedFullName().asUTF8();
}
}  // namespace doodle::maya_plug
