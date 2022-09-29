//
// Created by TD on 2022/5/10.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <doodle_app/gui/base/ref_base.h>
namespace doodle::gui {

class DOODLELIB_API redirection_path_info_edit : public edit_interface {
 private:
  class impl;

  std::unique_ptr<impl> ptr;

 public:
  redirection_path_info_edit();
  virtual ~redirection_path_info_edit() override;

 public:
  virtual void render(const entt::handle& in) override;

 protected:
  virtual void init_(const entt::handle& in) override;
  virtual void save_(const entt::handle& in) const override;
};

}  // namespace doodle::gui
