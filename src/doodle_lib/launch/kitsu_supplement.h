#pragma once

#include <doodle_lib/core/app_base.h>

namespace doodle {
class kitsu_supplement_main : public app_base {
 protected:
  using app_base::app_base;
  bool init() override;
};
}  // namespace doodle