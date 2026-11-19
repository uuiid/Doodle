#pragma once

#include <doodle_lib/launch/kitsu_supplement.h>

namespace doodle {
class kitsu_supplement_test_main : public kitsu_supplement_main {
 protected:
  bool init() override;

 public:
  using kitsu_supplement_main::kitsu_supplement_main;
};
}  // namespace doodle
