//
// Created by td_main on 2023/3/30.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <vector>
namespace doodle::gui {

class create_entry {
  std::vector<FSys::path> paths_{};

 public:
  explicit create_entry(const std::vector<FSys::path>& in_paths) : paths_(in_paths) {}

  constexpr static std::string_view name{gui::config::menu_w::create_entry_};

  bool render();
};

}  // namespace doodle::gui
