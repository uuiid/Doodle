//
// Created by TD on 2021/12/6.
//

#include "maya_file_io.h"

#include <maya/MFileIO.h>
namespace doodle::maya_plug {

FSys::path maya_file_io::get_current_path() const {
  auto k_s = MFileIO::currentFile();
  return {k_s.asUTF8()};
}
}  // namespace doodle::maya_plug
