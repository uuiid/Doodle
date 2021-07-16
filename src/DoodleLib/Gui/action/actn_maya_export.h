//
// Created by TD on 2021/7/16.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>

namespace doodle {

class DOODLELIB_API actn_maya_export : public action_indirect<action::arg_path> {
  actn_up_paths_ptr p_up_paths;
  FSys::path p_paths;

 public:
  actn_maya_export();

  bool is_async() override;
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
  bool is_accept(const arg_path& in_any) override;
};
}  // namespace doodle
