//
// Created by TD on 2022/1/13.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::core {
class DOODLELIB_API client {
 public:
  static void add_project(const FSys::path& in_path);
};
}  // namespace doodle::core
