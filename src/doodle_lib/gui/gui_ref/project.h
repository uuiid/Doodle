//
// Created by TD on 2022/1/20.
//

#pragma once
#include <doodle_lib/gui/gui_ref/ref_base.h>

namespace doodle {
namespace gui {

class DOODLELIB_API project_edit : public base_edit {
  class impl;
  std::unique_ptr<impl> p_i;

  virtual void init_(const entt::handle &in) override;
  virtual void save_(const entt::handle &in) const override;

 public:
  project_edit();
  ~project_edit();
  virtual void render(const entt::handle &in) override;
};

}  // namespace gui
}  // namespace doodle
