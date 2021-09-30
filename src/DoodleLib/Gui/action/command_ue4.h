//
// Created by TD on 2021/9/23.
//

#pragma once

#include <DoodleLib/Gui/action/command.h>
#include <DoodleLib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API comm_ass_ue4_create_shot : public command_meta {
 private:
  string_ptr p_ue4_prj_path;

  std::vector<shot_ptr> p_shot_list;

  ue4_project_async_ptr p_ue4;
  metadata_ptr p_parent;

 public:
  comm_ass_ue4_create_shot();
  bool render() override;
  bool add_data(const metadata_ptr& in_parent, const metadata_ptr& in) override;
};

class DOODLELIB_API comm_ass_ue4_import : public command_meta {
 private:
 public:
  comm_ass_ue4_import();
  bool render() override;
  bool add_data(const metadata_ptr& in_parent, const metadata_ptr& in) override;
};
}  // namespace doodle
