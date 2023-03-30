//
// Created by td_main on 2023/3/30.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include "entt/entity/fwd.hpp"
#include <functional>
#include <vector>
namespace doodle::gui {

class create_entry {
 public:
  class init_args {
    friend class create_entry;
    std::vector<FSys::path> paths_{};
    std::function<void(const std::vector<entt::handle>&)> fun_{};

   public:
    init_args() = default;

    inline init_args& set_create_call(const std::function<void(const std::vector<entt::handle>&)>& in_function) {
      fun_ = in_function;
      return *this;
    }
    inline init_args& set_paths(const std::vector<FSys::path>& in_paths) {
      paths_ = in_paths;
      return *this;
    };
  };

 private:
  init_args args_{};

 public:
  explicit create_entry(const init_args& in_args) : args_(in_args) {}

  constexpr static std::string_view name{gui::config::menu_w::create_entry_};
  static constexpr std::array<float, 2> sizexy{640, 360};
  bool render();
};

}  // namespace doodle::gui
