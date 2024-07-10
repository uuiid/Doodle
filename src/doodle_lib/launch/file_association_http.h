//
// Created by TD on 2023/12/29.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <argh.h>
namespace doodle::launch {
class file_association_http_t {
 public:
  file_association_http_t()  = default;
  ~file_association_http_t() = default;

  bool operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector);
};
}  // namespace doodle::launch