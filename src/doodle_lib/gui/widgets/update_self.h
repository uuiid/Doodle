//
// Created by td_main on 2023/3/31.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle::gui {

class update_self {
 public:
  bool render();

  constexpr static std::string_view name{gui::config::menu_w::create_entry_};
  static constexpr std::array<float, 2> sizexy{640, 360};
};

}  // namespace doodle::gui
