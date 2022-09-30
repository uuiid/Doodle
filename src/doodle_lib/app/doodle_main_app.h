
//
// Created by TD on 2022/1/18.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_app/app/app_command.h>
#include <doodle_app/app/facet/gui_facet.h>
namespace doodle {

class DOODLELIB_API main_facet : public facet::gui_facet {
 public:
 protected:
  void load_windows() override;
};

class DOODLELIB_API main_app : public doodle_main_app {
 public:
  explicit main_app(const in_gui_arg& in_arg);
};
}  // namespace doodle
