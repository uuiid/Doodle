//
// Created by TD on 2021/8/4.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <nana/gui/widgets/menu.hpp>

namespace doodle {

class DOODLELIB_API tool_box_menu_factory
    : public std::enable_shared_from_this<tool_box_menu_factory> {
  std::vector<action_base_ptr> p_list;
  nana::window p_window;

  void create_menu();

 public:
  tool_box_menu_factory(nana::window in_window);

  void operator()(nana::menu& in_menu);
};
}  // namespace doodle
