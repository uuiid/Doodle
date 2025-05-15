//
// Created by TD on 25-5-15.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle::details {

class sqlite_upgrade_impl {
 public:
  virtual ~sqlite_upgrade_impl() = default;

  virtual void upgrade()         = 0;
};

}  // namespace doodle::details