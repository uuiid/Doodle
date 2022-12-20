#pragma once

#include "configure/doodle_lib_export.h"
#include "doodle_lib/doodle_lib_fwd.h"

#include <memory>
#include <string>
#include <vector>

namespace doodle::detail {

class DOODLELIB_API xlsx_file {
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  // xlsx_file();
  // ~xlsx_file();

  // void open(const FSys::path& in_path);

  // void write_header(const std::vector<std::string>& in);
};

}  // namespace doodle::detail