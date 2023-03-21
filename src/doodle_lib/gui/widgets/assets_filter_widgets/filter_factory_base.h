//
// Created by TD on 2022/5/7.
//

#pragma once

#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui {
class filter_base;

class DOODLELIB_API filter_factory_base {
  class impl;
  std::unique_ptr<impl> p_i;

  void connection_sig();

 protected:
  virtual std::unique_ptr<filter_base> make_filter_() = 0;
  virtual void refresh_()                             = 0;
  virtual void init()                                 = 0;

  bool is_disabled;

 public:
  filter_factory_base();
  virtual ~filter_factory_base();
  entt::observer p_obs;
  bool is_edit;
  virtual bool render() = 0;
  virtual void refresh(bool force);
  std::unique_ptr<filter_base> make_filter();
};

}  // namespace doodle::gui
