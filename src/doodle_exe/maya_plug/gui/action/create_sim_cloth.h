//
// Created by TD on 2021/12/20.
//

#pragma once
#include <doodle_lib/gui/action/command.h>
namespace doodle::maya_plug {
class create_sim_cloth : public command_base {
  std::vector<entt::handle> p_list;

 public:
  create_sim_cloth();

  bool render() override;
};
}  // namespace doodle::maya_plug
