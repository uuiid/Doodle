//
// Created by TD on 2024/3/1.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <argh.h>
namespace doodle {
class http_working_service_t {
 public:
  http_working_service_t()  = default;
  ~http_working_service_t() = default;

  bool operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector);
};
}  // namespace doodle