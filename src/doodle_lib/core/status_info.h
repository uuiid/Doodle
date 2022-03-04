//
// Created by TD on 2022/2/16.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

class DOODLELIB_API status_info final {
 public:
  bool need_save;
  std::string message;

  std::size_t show_size;
  std::size_t select_size;
};

}  // namespace doodle
