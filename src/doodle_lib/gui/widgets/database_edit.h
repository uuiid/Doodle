//
// Created by TD on 2022/2/15.
//

#pragma once
#include <doodle_app/gui/base/ref_base.h>

namespace doodle::gui {

class DOODLELIB_API database_edit : public edit_interface {
  class impl;
  std::unique_ptr<impl> p_i;

  void init_(const entt::handle &in) override;
  void save_(const entt::handle &in) const override;

  void format_();

 public:
  database_edit();
  ~database_edit();
  void render(const entt::handle &in) override;
  void link_sig(const std::unique_ptr<edit_interface> &in_unique_ptr);
};

}  // namespace doodle::gui
