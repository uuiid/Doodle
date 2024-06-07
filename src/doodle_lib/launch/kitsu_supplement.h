#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <argh.h>

namespace doodle::launch {
class kitsu_supplement_t {
 public:
  kitsu_supplement_t()  = default;
  ~kitsu_supplement_t() = default;

  bool operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector);
};
}  // namespace doodle::launch