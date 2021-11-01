//
// Created by TD on 2021/9/23.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>

#include <boost/signals2.hpp>
namespace doodle {
class DOODLELIB_API edit_widgets  : public base_widget{
  entt::entity p_meta_parent;

  registry_ptr p_reg;
  public:
  edit_widgets();
  virtual void frame_render() override;

  void set_factort(const attribute_factory_ptr& in_factory);
};
}  // namespace doodle
