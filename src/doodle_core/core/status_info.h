//
// Created by TD on 2022/2/16.
//

#pragma once
#include <doodle_core/configure/doodle_core_export.h>
#include <doodle_core/doodle_core_fwd.h>

namespace doodle {

class DOODLE_CORE_API status_info final {
 public:
  bool need_save;
  std::string message;

  std::size_t show_size;
  std::size_t select_size;
};

}  // namespace doodle
