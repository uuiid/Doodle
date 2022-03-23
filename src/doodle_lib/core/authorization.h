//
// Created by TD on 2022/3/23.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API authorization {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  explicit authorization(FSys::path in_path);
  ~authorization();

  bool is_expire() const;

  static void generate_token(const FSys::path& in_path);
};
}  // namespace doodle
