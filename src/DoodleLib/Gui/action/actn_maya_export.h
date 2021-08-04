//
// Created by TD on 2021/7/16.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>

namespace doodle {

class DOODLELIB_API actn_maya_export : public action_indirect<action_arg::arg_path> {
  actn_create_ass_up_paths_ptr p_up_paths;
  FSys::path p_paths;

 public:
  actn_maya_export();
  using arg = action_arg::arg_path;

  bool is_async() override;
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
  bool is_accept(const action_arg::arg_path& in_any) override;
};

class DOODLELIB_API actn_maya_export_batch : public action_indirect<action_arg::arg_null> {
  actn_create_ass_up_paths_ptr p_up_paths;
  FSys::path p_paths;

 public:
  actn_maya_export_batch();
  using arg = action_arg::arg_null;

  bool is_async() override;
  long_term_ptr run(const MetadataPtr& in_data, const MetadataPtr& in_parent) override;
};
}  // namespace doodle
