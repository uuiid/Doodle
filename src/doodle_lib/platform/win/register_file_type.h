//
// Created by TD on 2022/2/22.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/project.h>

namespace doodle {

class register_file_type {
  static std::string get_key_str();

 public:
  register_file_type();

  static FSys::path program_location();
};

}  // namespace doodle
