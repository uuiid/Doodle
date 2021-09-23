//
// Created by TD on 2021/9/23.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/command.h>

namespace doodle {
class DOODLELIB_API comm_project_add : public command_meta {
 private:
  string_ptr p_prj_name;
  string_ptr p_prj_name_short;
  string_ptr p_prj_path;
 public:
  comm_project_add();
  bool run(const MetadataPtr& in_parent, const MetadataPtr &in) override;
};

}  // namespace doodle
