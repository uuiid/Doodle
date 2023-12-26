//
// Created by TD on 2023/12/26.
//

#pragma once

#include <doodle_lib/core/scan_win_service.h>
namespace doodle {

class windwos_service_auto_light_facet_t {
  std::shared_ptr<sac>

      public : windwos_service_auto_light_facet_t() = default;
  ~windwos_service_auto_light_facet_t()             = default;

  bool post();
  void add_program_options(){};
};

}  // namespace doodle