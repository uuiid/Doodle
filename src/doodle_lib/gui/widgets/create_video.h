//
// Created by TD on 2022/9/21.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_core/core/init_register.h>
#include <doodle_lib/gui/gui_ref/base_window.h>
namespace doodle::gui {

class DOODLELIB_API create_video
    : public gui::base_windows<
          dear::Begin,
          create_video> {
  class impl;
  class image_arg;
  std::unique_ptr<impl> p_i;

  entt::handle create_image_to_move_handle(const FSys::path& in_path);

 public:
  create_video();
  ~create_video() override;
  constexpr static std::string_view name{gui::config::menu_w::comm_create_video};
  const std::string& title() const override;
  void render();
};
}  // namespace doodle::gui
