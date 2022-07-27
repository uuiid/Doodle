//
// Created by TD on 2021/12/20.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>
#include <maya_plug/configure/static_value.h>

namespace doodle::maya_plug {
class create_sim_cloth
    : public gui::window_panel {
  std::vector<entt::handle> p_list;

  entt::handle p_coll;

  void run_comm();


 public:
  create_sim_cloth();
  ~create_sim_cloth() override;

  constexpr static auto name = ::doodle::gui::config::maya_plug::menu::create_sim_cloth;

  void render() override;
};
namespace create_sim_cloth_ns {
constexpr auto init = []() {
  entt::meta<create_sim_cloth>()
      .type()
      .prop("name"_hs, std::string{create_sim_cloth::name})
      .base<gui::window_panel>();
};
class init_class
    : public init_register::registrar_lambda<init, 3> {};
}  // namespace create_sim_cloth_ns
}  // namespace doodle::maya_plug
