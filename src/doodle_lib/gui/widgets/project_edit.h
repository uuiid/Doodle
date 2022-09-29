//
// Created by TD on 2022/2/7.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_app/gui/base/base_window.h>

namespace doodle::gui {

class DOODLELIB_API project_edit
    : public base_windows<
          dear::Begin, project_edit> {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  project_edit();
  ~project_edit() override;

  constexpr static std::string_view name{gui::config::menu_w::project_edit};

  void init();
  const std::string& title() const override;
  void render();
};

}  // namespace doodle::gui
