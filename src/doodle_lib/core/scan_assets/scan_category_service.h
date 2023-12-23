//
// Created by TD on 2023/12/23.
//

#pragma once
#include <doodle_lib/core/scan_assets/base.h>

namespace doodle::details {

class scan_category_service_t {
 public:
  scan_category_service_t()          = default;
  virtual ~scan_category_service_t() = default;
  template <typename CompletionHandler>
  auto async_scan_files(
      scan_category_data_t::project_root_t in_project_root, std::shared_ptr<scan_category_t> in_scan_category_ptr,
      CompletionHandler &&in_completion
  ) {}
};

}  // namespace doodle::details