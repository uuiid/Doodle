//
// Created by TD on 2022/8/9.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <doodle_app/gui/base/ref_base.h>

namespace doodle {
namespace gui {

class DOODLELIB_API edit_user : public edit_interface {
 private:
  class impl;

  std::unique_ptr<impl> ptr;

 public:
  edit_user();
  virtual ~edit_user();

 public:
  virtual void render(const entt::handle& in) override;

 protected:
  virtual void init_(const entt::handle& in) override;
  virtual void save_(const entt::handle& in) const override;
};

}  // namespace gui
}  // namespace doodle
