//
// Created by td_main on 2023/11/7.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>

namespace doodle::gui {
class open_project {
  bool open{true};

 public:
  open_project()  = default;
  ~open_project() = default;
  constexpr static std::string_view name{"打开项目"};
  static constexpr std::array<float, 2> sizexy{640, 360};
  bool render();
};

}  // namespace doodle::gui
