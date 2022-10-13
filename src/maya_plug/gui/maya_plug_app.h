//
// Created by TD on 2021/10/14.
//

#pragma once
#include <doodle_core/core/app_facet.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/app/facet/gui_facet.h>
namespace doodle::maya_plug {


class maya_facet : public doodle::facet::gui_facet {
 protected:
  void load_windows() override;

 public:
  maya_facet();
  void close_windows() override;
};

class maya_plug_app : public doodle::doodle_main_app {
  void set_facet();

 public:
  maya_plug_app();
  explicit maya_plug_app(const in_gui_arg& inArg);
};

}  // namespace doodle::maya_plug
