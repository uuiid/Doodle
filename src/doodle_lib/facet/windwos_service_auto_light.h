//
// Created by TD on 2023/12/26.
//

#pragma once

#include <doodle_lib/core/scan_win_service.h>
namespace doodle {

class windows_service_auto_light_facet_t {
  std::shared_ptr<scan_win_service_t> scan_win_service_ptr_;

 public:
  windows_service_auto_light_facet_t()  = default;
  ~windows_service_auto_light_facet_t() = default;

  bool post();
  void add_program_options();
  static void install_server();
};

}  // namespace doodle