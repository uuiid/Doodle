//
// Created by TD on 2022/1/20.
//

#pragma once
#include <doodle_lib/gui/gui_ref/ref_base.h>

namespace doodle::gui {

class DOODLELIB_API project_edit : public edit_interface {
  class impl;
  std::unique_ptr<impl> p_i;

  void init_(const entt::handle &in) override;
  void save_(const entt::handle &in) const override;

 public:
  project_edit();
  ~project_edit();
  void render(const entt::handle &in) override;
};

class DOODLELIB_API cloth_config_edit : public edit_interface {
  class impl;
  std::unique_ptr<impl> p_i;
  void init_(const entt::handle &in) override;
  void save_(const entt::handle &in) const override;

 public:
  cloth_config_edit();
  ~cloth_config_edit();
  void render(const entt::handle &in) override;
};

}  // namespace doodle::gui
