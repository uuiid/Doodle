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
  leaf_meta()  = default;
  ~leaf_meta() = default;

  virtual void set_metadata(const std::weak_ptr<metadata>& in_meta);
  virtual std::weak_ptr<metadata> get_metadata() const;

  template <class T>
  std::shared_ptr<T> get_meta() const {
    return std::dynamic_pointer_cast<T>(get_metadata().lock());
  }

  // virtual void set_metadata(const std::weak_ptr<metadata>& in_meta);
};
}  // namespace doodle
