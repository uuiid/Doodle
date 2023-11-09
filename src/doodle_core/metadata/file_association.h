//
// Created by td_main on 2023/11/9.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle {

class file_association {
 public:
  file_association()          = default;
  virtual ~file_association() = default;

  entt::handle maya_file;
  entt::handle maya_rig_file;
  entt::handle ue_file;
  entt::handle ue_preset_file;

  //  void set_maya_file(const entt::handle& in_maya_file);
  //  void set_maya_rig_file(const entt::handle& in_maya_rig_file);
  //  void set_ue_file(const entt::handle& in_ue_file);
  //  [[nodiscard]] const entt::handle& get_maya_file() const;
  //  [[nodiscard]] const entt::handle& get_maya_rig_file() const;
  //  [[nodiscard]] const entt::handle& get_ue_file() const;
};

}  // namespace doodle