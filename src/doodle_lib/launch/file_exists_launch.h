//
// Created by TD on 24-4-23.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <argh.h>
namespace doodle::launch {

class file_exists_launch_t {
  static constexpr auto g_file_path = "file_path";

 public:
  file_exists_launch_t()  = default;
  ~file_exists_launch_t() = default;

  bool operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector);
};

}  // namespace doodle::launch
}