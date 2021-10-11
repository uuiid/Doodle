//
// Created by teXiao on 2021/4/27.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API leaf_meta {
 protected:
 std::weak_ptr<metadata> p_meta;
 public:
  leaf_meta() = default;
  ~leaf_meta() =default;

  virtual void set_metadata(const std::weak_ptr<metadata>& in_meta);
  //virtual void set_metadata(const std::weak_ptr<metadata>& in_meta);
};
}  // namespace doodle
