//
// Created by teXiao on 2021/4/27.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
class DOODLELIB_API leaf_meta {
 protected:
 std::weak_ptr<metadata> p_meta;
 public:
  leaf_meta() = default;
  ~leaf_meta() =default;

  virtual void set_metadata(const metadata_ptr& in_meta);
};
}  // namespace doodle
