
//
// Created by TD on 2022/1/18.
//

#pragma once
#include <doodle_app/app/app_command.h>
#include <doodle_app/app/facet/gui_facet.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

// namespace detail {
// class DOODLELIB_API rpc_server_facet : public ::doodle::facet::json_rpc_facet {
//
//  public:
// };
// }  // namespace detail

class DOODLELIB_API main_facet : public facet::gui_facet {
 public:
  main_facet();

 protected:
  void load_windows() override;
};

class DOODLELIB_API main_app : public doodle_main_app {
 public:
  main_app();
  explicit main_app(const in_gui_arg& in_arg);

 protected:
  bool chick_authorization() override;
};
}  // namespace doodle
