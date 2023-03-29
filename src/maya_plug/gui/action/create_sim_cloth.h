//
// Created by TD on 2021/12/20.
//

#pragma once

#include <doodle_app/gui/base/base_window.h>

#include <maya_plug/configure/static_value.h>

namespace doodle::maya_plug {
class create_sim_cloth {
  std::vector<entt::handle> p_list;

  entt::handle p_coll;
  std::string title_name_;
  bool open{true};
  void run_comm();

 public:
  create_sim_cloth();
  ~create_sim_cloth();

  constexpr static auto name = ::doodle::gui::config::maya_plug::menu::create_sim_cloth;

  bool render();
  const std::string& title() const;
};

}  // namespace doodle::maya_plug
