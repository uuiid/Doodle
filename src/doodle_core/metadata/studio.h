//
// Created by TD on 25-3-18.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
namespace doodle {
struct DOODLE_CORE_API studio   {
  DOODLE_BASE_FIELDS();
        std::string name_;
        std::string color_;
        bool active_;
};
}  // namespace doodle