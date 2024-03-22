//
// Created by TD on 2024/3/22.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <argh.h>
namespace doodle::launch {
class auto_light_process_t {
 public:
  auto_light_process_t()  = default;
  ~auto_light_process_t() = default;

  bool operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector);
};
}  // namespace doodle::launch
