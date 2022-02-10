//
// Created by TD on 2022/2/10.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

class short_cut : public process_t<short_cut> {
 public:
  short_cut();

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const delta_type&, void* data);
};

}  // namespace doodle
