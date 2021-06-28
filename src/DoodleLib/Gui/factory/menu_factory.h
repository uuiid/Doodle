//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <nana/gui.hpp>
namespace doodle {

class DOODLELIB_API menu_factory {
  CoreSet& p_set;
 public:
  explicit  menu_factory(nana::window in_window);
};
}  // namespace doodle
