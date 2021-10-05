//
// Created by TD on 2021/9/23.
//

#pragma once

#include <doodle_lib/Gui/action/command.h>
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API comm_ass_ue4_create_shot : public command_base {
 private:
  string_ptr p_ue4_prj_path;

  std::vector<shot_ptr> p_shot_list;

  ue4_project_async_ptr p_ue4;
  metadata_ptr p_parent;

 public:
  comm_ass_ue4_create_shot();
  bool render() override;
  bool add_data(const metadata_ptr& in_parent, const metadata_ptr& in);
};

class DOODLELIB_API comm_ass_ue4_import : public command_base {
 private:
 public:
  comm_ass_ue4_import();
  bool render() override;
  bool add_data(const metadata_ptr& in_parent, const metadata_ptr& in);
};
}  // namespace doodle
