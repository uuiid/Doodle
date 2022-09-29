
//
// Created by TD on 2022/1/18.
//

#pragma once
#include <doodle_app/app/app.h>

namespace doodle {
class DOODLELIB_API main_app : public doodle_main_app {
 public:
 protected:
  void load_windows() override;
};
}  // namespace doodle
