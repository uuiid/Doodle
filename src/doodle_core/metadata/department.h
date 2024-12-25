//
// Created by TD on 24-12-25.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
namespace doodle {
struct DOODLE_CORE_API department : base {
  std::string name_;
  std::string color_;
  bool archived_;
};
}  // namespace doodle