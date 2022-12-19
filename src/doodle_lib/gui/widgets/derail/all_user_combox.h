//
// Created by TD on 2022/8/4.
//

#pragma once

#include <doodle_app/gui/base/modify_guard.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <entt/entity/fwd.hpp>
#include <iterator>
#include <memory>
#include <string>

namespace doodle::gui {
class DOODLELIB_API all_user_combox {
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  all_user_combox();
  ~all_user_combox();

  bool render();
  entt::handle get_user();
};
}  // namespace doodle::gui