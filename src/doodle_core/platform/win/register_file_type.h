//
// Created by TD on 2022/2/22.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/metadata.h>
namespace doodle {

class register_file_type {
 public:
  register_file_type();

  void register_type();

  std::optional<database::ref_data> get_ref_uuid();
};

}  // namespace doodle
