//
// Created by TD on 2023/12/29.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <argh.h>
namespace doodle::launch {
class auto_light_service_t {
 public:
  auto_light_service_t()  = default;
  ~auto_light_service_t() = default;

  bool operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector);
};
}  // namespace doodle::launch