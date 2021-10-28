//
// Created by TD on 2021/9/23.
//

#pragma once

#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API comm_ass_ue4_create_shot : public command_base_tool {
 private:
  string_ptr p_ue4_prj_path;

  std::vector<entt::handle> p_shot_list;

  ue4_project_async_ptr p_ue4;

 public:
  comm_ass_ue4_create_shot();
  bool render() override;

};

class DOODLELIB_API comm_ass_ue4_import : public command_base_tool {
 private:
 public:
  comm_ass_ue4_import();
  bool render() override;
};
}  // namespace doodle
