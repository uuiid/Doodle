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

  entt::handle maya_file{};       /// maya文件
  entt::handle maya_rig_file{};   /// maya绑定文件
  entt::handle maya_sim_file{};   /// maya解算文件
  entt::handle ue_file{};         /// ue文件
  entt::handle ue_preset_file{};  /// ue预设文件
  std::string name{};
};

}  // namespace doodle